//end namespace gfx
// This file is part of CaesarIA.
//
// CaesarIA is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// CaesarIA is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with CaesarIA.  If not, see <http://www.gnu.org/licenses/>.
//
// Copyright 2012-2014 Dalerank, dalerankn8@gmail.com

#include "layer.hpp"
#include "objects/overlay.hpp"
#include "city/statistic.hpp"
#include "game/resourcegroup.hpp"
#include "gfx/tilesarray.hpp"
#include "objects/building.hpp"
#include "objects/tree.hpp"
#include "core/variant_map.hpp"
#include "core/event.hpp"
#include "gfx/city_renderer.hpp"
#include "events/showtileinfo.hpp"
#include "walker/constants.hpp"
#include "walker/walker.hpp"
#include "gfx/tilemap_camera.hpp"
#include "city/city.hpp"
#include "core/font.hpp"
#include "constants.hpp"
#include "gfx/decorator.hpp"
#include "core/utils.hpp"
#include "core/osystem.hpp"
#include "gfx/walker_debuginfo.hpp"
#include "core/timer.hpp"
#include "core/logger.hpp"
#include "core/color_list.hpp"
#include "gfx/animation_bank.hpp"
#include "game/settings.hpp"
#include "city/statistic.hpp"

using namespace gfx;

namespace citylayer
{

class Layer::Impl
{
public:
  typedef std::set<object::Type> AlwaysDrawObjects;

  struct
  {
    Point last;
    Point start;
  } cursor;

  struct {
    bool buildings = true;
    bool trees = true;
  } draw;

  struct {
    Picture image;
    std::string text;
  } tooltip;

  int nextLayer;
  struct {
    Layer::WalkerTypes walkers;
    AlwaysDrawObjects objects;
  } visible;

  struct PictureInfo {
    Point pos;
    Picture pic;
  };

  typedef std::vector<PictureInfo> Pictures;
  typedef std::vector<LayerDrawPassPtr> Passes;
  typedef std::map<int,Passes> PassesMap;

  Pictures pictures;
  Picture terraintPic;
  Picture greenTile;
  PlayerCityPtr city;
  Picture tilePosText;
  Tile* currentTile;
  Picture outline;
  Camera* camera;
  Font debugFont;
  PassesMap passes;

  int posMode;
};

void Layer::registerTileForRendering(Tile& tile)
{
}

void Layer::renderUi( Engine& engine )
{
  __D_REF(d,Layer)
  if( !d.tooltip.text.empty() )
  {
    engine.draw( d.tooltip.image, d.cursor.last );
  }

  const auto& passes = d.passes[ LayerDrawPass::gui ];
  Tile* tile = _currentTile();
  if( !passes.empty() && tile != nullptr  )
  {
    RenderInfo rinfo = { engine, d.camera->offset() };
    for( auto pass : passes )
      pass->draw( rinfo, *tile );
  }
}

void Layer::handleEvent(NEvent& event)
{
  __D_IMPL(_d,Layer)
  if( event.EventType == sEventMouse )
  {
    Point pos = event.mouse.pos();

    switch( event.mouse.type  )
    {
    case mouseMoved:
    {
      Point savePos = _d->cursor.last;
      bool movingPressed = _isMovingButtonPressed( event );
      _d->cursor.last = pos;

      if( !movingPressed || _d->cursor.start.x() < 0 )
      {
        _d->cursor.start = _d->cursor.last;
      }

      if( movingPressed )
      {
        Point delta = _d->cursor.last - savePos;
        _d->camera->move( PointF( -delta.x() * 0.1, delta.y() * 0.1 ) );
      }

      Tile* selectedTile = _d->camera->at( _d->cursor.last, true );
      _d->currentTile = selectedTile;
    }
    break;

    case mouseLbtnPressed:
    {
      _d->cursor.start = _d->cursor.last;
    }
    break;

    case mouseLbtnRelease:            // left button
    case mouseMbtnRelease:
    {
      Tile* tile = _d->camera->at( pos, false );  // tile under the cursor (or NULL)
      if( tile == 0 )
      {
        break;
      }

      if( event.mouse.control )
      {
        _d->camera->setCenter( tile->pos() );
        _d->city->setCameraPos( tile->pos() );
      }

      _d->cursor.start = _d->cursor.last;
    }
    break;

    case mouseRbtnRelease:
    {
      Tile* tile = _d->camera->at( pos, false );  // tile under the cursor (or NULL)
      if( tile )
      {
        events::dispatch<events::ShowTileInfo>( tile->epos() );
      }
    }
    break;

    default:
    break;
    }
  }
  else if( event.EventType == sEvenApplication )
  {
    switch( event.app.type )
    {
    case appWindowFocusEnter:
    case appWindowFocusLeave:
      DrawOptions::instance().setFlag( DrawOptions::windowActive, event.app.type == appWindowFocusEnter );
    break;

    default: break;
    }
  }
  else if( event.EventType == sEventKeyboard )
  {
    bool handled = _moveCamera( event );
    if( !handled )
    {
      switch( event.keyboard.key )
      {
      case KEY_ESCAPE: _setNextLayer( citylayer::simple ); break;
      default: break;
      }
    }
  }
}

TilesArray Layer::_getSelectedArea( TilePos startPos )
{
  __D_IMPL(_d,Layer)
  TilePos outStartPos, outStopPos;

  Tile* startTile = startPos.i() < 0
                      ? _d->camera->at( _d->cursor.start, true ) // tile under the cursor (or NULL)
                      : _d->camera->at( startPos );
  Tile* stopTile  = _d->camera->at( _d->cursor.last, true );

  TilePos startPosTmp = startTile->epos();
  TilePos stopPosTmp  = stopTile->epos();

  outStartPos = TilePos( std::min<int>( startPosTmp.i(), stopPosTmp.i() ), std::min<int>( startPosTmp.j(), stopPosTmp.j() ) );
  outStopPos  = TilePos( std::max<int>( startPosTmp.i(), stopPosTmp.i() ), std::max<int>( startPosTmp.j(), stopPosTmp.j() ) );

  return _city()->tilemap().area( outStartPos, outStopPos );
}

void Layer::drawPass( const RenderInfo& rinfo, Tile& tile, Renderer::Pass pass)
{
  Picture refPic;
  bool find = false;
  switch( pass )
  {
  case Renderer::ground:
    refPic = tile.picture();
    find = true;
  break;

  case Renderer::groundAnimation:
    if( tile.animation().isValid() )
    {
      refPic = tile.animation().currentFrame();
      find = true;
    }
  break;

  case Renderer::overlay:
    if( tile.rov().isValid() )
    {
      find = true;
      refPic = tile.rov()->picture();
    }
  break;

  default:
  break;
  }

  Point screenPos = tile.mappos() + rinfo.offset;
  if( refPic.isValid() )
  {
    rinfo.engine.draw( refPic, screenPos );
  }

  if( !find && tile.rov().isValid() )
  {
    const Pictures& pics = tile.rov()->pictures( pass );
    rinfo.engine.draw( pics, screenPos );
  }
}

void Layer::drawWalkers( const RenderInfo& rinfo, const Tile& tile)
{
  Pictures pics;
  const WalkerList& walkers = _city()->walkers( tile.epos() );
  const Layer::WalkerTypes& vWalkers = visibleTypes();

  bool viewAll = vWalkers.count( walker::all );

  for( auto wlk : walkers )
  {
    if( viewAll || vWalkers.count( wlk->type() ) > 0 )
    {
      pics.clear();
      wlk->getPictures( pics );
      rinfo.engine.draw( pics, wlk->mappos() + rinfo.offset );
    }
  }
}

void Layer::_setTooltipText(const std::string& text)
{
  __D_REF(d,Layer)
  if( d.tooltip.text != text )
  {
    Font font = Font::create( FONT_2 );
    d.tooltip.text = text;
    Size size = font.getTextSize( text );

    if( d.tooltip.image.isValid() || (d.tooltip.image.size() != size) )
    {
      d.tooltip.image = Picture( size, 0, true );
    }

    d.tooltip.image.fill( 0x00000000, Rect( Point( 0, 0 ), d.tooltip.image.size() ) );
    d.tooltip.image.fill( 0xff000000, Rect( Point( 0, 0 ), size ) );
    d.tooltip.image.fill( 0xffffffff, Rect( Point( 1, 1 ), size - Size( 2, 2 ) ) );
    font.draw( d.tooltip.image, text, Point(), false );
  }
}

void Layer::render( Engine& engine )
{
  __D_REF(d,Layer)
  const TilesArray& visibleTiles = d.camera->tiles();
  RenderInfo rinfo = { engine, d.camera->offset() };

  _camera()->startFrame();
  DrawOptions& opts = DrawOptions::instance();
  //FIRST PART: draw lands
  drawLands( rinfo, d.camera );

  if( opts.isFlag( DrawOptions::shadowOverlay ) )
  {
    engine.setColorMask( 0x00ff0000, 0x0000ff00, 0x000000ff, 0xc0000000 );
  }
  // SECOND PART: draw all sprites, impassable land and buildings
  //int r0=0, r1=0, r2=0;
  for( auto tile : visibleTiles )
  {
    int z = tile->epos().z();

    drawProminentTile( rinfo, *tile, z, false );
    drawWalkers( rinfo, *tile );
    drawWalkerOverlap( rinfo, *tile, z );
  }

  engine.resetColorMask();

  if( opts.isFlag( DrawOptions::showPath ) )
  {
    WalkerList overDrawWalkers;

    const WalkerList& walkers = _city()->walkers();
    for( auto wlk : walkers )
    {
      if( wlk->getFlag( Walker::showPath ) )
      {
        overDrawWalkers << wlk;
      }
      else
      {
        if( wlk->getFlag( Walker::showDebugInfo ) )
          WalkerDebugInfo::showPath( wlk, rinfo );
      }
    }

    for( auto it : overDrawWalkers )
    {
      WalkerDebugInfo::showPath( it, rinfo, ColorList::yellow );
    }
  }
}

void Layer::drawWalkerOverlap( const RenderInfo& rinfo, Tile& tile, const int depth)
{
  Tile* master = tile.master();

  // multi-tile: draw the master tile.
  // single-tile: draw current tile
  // and it is time to draw the master tile
  drawPass( rinfo, 0 == master ? tile : *master, Renderer::overWalker );
}

const Layer::WalkerTypes& Layer::visibleTypes() const
{
  return _dfunc()->visible.walkers;
}

void Layer::drawProminentTile( const RenderInfo& rinfo, Tile& tile, const int depth, bool force)
{
  if( tile.isFlat() && !force )
  {
    return;  // tile has already been drawn!
  }

  Tile* master = tile.master();

  if( 0 == master )    // single-tile
  {
    drawTile( rinfo, tile );
    return;
  }

  // multi-tile: draw the master tile.
  // and it is time to draw the master tile
  if( !master->rendered() && master == &tile )
  {
    drawTile( rinfo, *master );
  }
}

void Layer::drawTile(const RenderInfo& rinfo, Tile& tile)
{
  __D_IMPL(_d,Layer)
  if( !tile.rendered() )
  {
    if( tile.rov().isValid() )
    {
      registerTileForRendering( tile );

      bool breakBuilding = is_kind_of<Building>( tile.rov() ) && !_d->draw.buildings;
      bool breakTree = is_kind_of<Tree>( tile.rov() ) && !_d->draw.trees;

      if( !(breakBuilding || breakTree) )
        drawOverlayedTile( rinfo, tile );
    }
    else
    {
      drawLandTile( rinfo, tile );
    }

    tile.setRendered();
  }
}

void Layer::drawOverlayedTile( const RenderInfo& rinfo, Tile& tile )
{
  drawPass( rinfo, tile, Renderer::overlayGround    );
  drawPass( rinfo, tile, Renderer::overlay          );
  drawPass( rinfo, tile, Renderer::overlayAnimation );
}

void Layer::drawArea( const RenderInfo& rinfo, const TilesArray& area, const std::string &resourceGroup, int tileId)
{
  if( area.empty() )
    return;

  if( area.size() == 1 )
  {
    Picture pic(resourceGroup, tileId );
    rinfo.engine.draw( pic, area.front()->mappos() + rinfo.offset );
  }
  else
  {
    Tile* baseTile = area.front();
    OverlayPtr overlay = baseTile->overlay();
    int leftBorderAtI = baseTile->i();
    int rightBorderAtJ = overlay.isValid()
                            ? overlay->size().height() - 1 + baseTile->j()
                            : baseTile->j();
    for( auto tile : area )
    {
      int tileBorders = ( tile->i() == leftBorderAtI ? 0 : config::tile.skipLeftBorder )
                        + ( tile->j() == rightBorderAtJ ? 0 : config::tile.skipRightBorder );
      Picture pic(resourceGroup, tileBorders + tileId);
      rinfo.engine.draw( pic, tile->mappos() + rinfo.offset );
    }
  }
}

void Layer::drawLands( const RenderInfo& rinfo, Camera* camera )
{
  const TilesArray& flatTiles = camera->flatTiles();
  const TilesArray& groundTiles = camera->groundTiles();

  // FIRST PART: draw all flat land (walkable/boatable)
  for( auto tile : groundTiles )
    drawLandTile( rinfo, *tile );

  for( auto tile : flatTiles )
    drawFlatTile( rinfo, *tile );
}

void Layer::drawLandTile(const RenderInfo& rinfo, Tile &tile)
{
  drawPass( rinfo, tile, Renderer::ground );
  drawPass( rinfo, tile, Renderer::groundAnimation );
}

void Layer::drawFlatTile(const RenderInfo& rinfo, Tile& tile)
{
  if( tile.rov().isValid() )
    drawTile( rinfo, tile );
}

void Layer::init( Point cursor )
{
  __D_REF(d,Layer)
  d.cursor.last = cursor;
  d.cursor.start = cursor;
  d.nextLayer = type();
}

void Layer::beforeRender(Engine&)
{
  __D_REF(d,Layer)
  d.draw.buildings = DrawOptions::instance().isFlag( DrawOptions::showBuildings );
  d.draw.trees = DrawOptions::instance().isFlag( DrawOptions::showTrees );
  d.pictures.clear();
}

void Layer::_addPicture(const Point& pos, const Picture& pic)
{
  Impl::PictureInfo info = { pos, pic };
  _dfunc()->pictures.push_back( info );
}

void Layer::_addText(const Point& pos, const std::string& text, Font font)
{
  if( !font.isValid() )
    font = _dfunc()->debugFont;

  Picture tx = font.once( text );
  _addPicture( pos, tx );
}

void Layer::afterRender(Engine& engine)
{
  __D_REF(_d,Layer)
  Point cursorPos = engine.cursorPos();
  Size screenSize = engine.viewportSize();
  Point offset = _d.camera->offset();
  Point moveValue;

  //on edge cursor moving
  DrawOptions& opts = DrawOptions::instance();
  if( opts.isFlag( DrawOptions::windowActive | DrawOptions::borderMoving ) )
  {
    if( cursorPos.x() >= 0 && cursorPos.x() < 2 ) moveValue.rx() -= 1;
    else
      if( cursorPos.x() > screenSize.width() - 2 )
        moveValue.rx() += 1;

    if( cursorPos.y() >= 0 && cursorPos.y() < 2 ) moveValue.ry() += 1;
    else
      if( cursorPos.y() > screenSize.height() - 2 )
        moveValue.ry() -= 1;

    if( moveValue.x() != 0 || moveValue.y() != 0 )
    {
      _d.camera->move( moveValue.toPointF() );
    }
  }

  if( opts.isFlag( DrawOptions::drawGrid ) )
  {
    Tilemap& tmap = _d.city->tilemap();
    int size = tmap.size();
    //Picture& screen = engine.screen();
    for( int k=0; k < size; k++ )
    {
      const Tile& tile = tmap.at( 0, k );
      const Tile& etile = tmap.at( size - 1, k );
      NColor color = ColorList::red;
      color.setAlpha( k%10 ? 0x80 : 0x40 );
      engine.drawLine( color, tile.mappos() + offset, etile.mappos() + offset );

      const Tile& rtile = tmap.at( k, 0 );
      const Tile& ertile = tmap.at( k, size - 1 );
      engine.drawLine( color, rtile.mappos() + offset, ertile.mappos() + offset );
    }
  }

  if( opts.isFlag( DrawOptions::showRoads ) )
  {
    const TilesArray& tiles = _d.camera->tiles();
    for( auto tile : tiles )
    {
      if( tile->getFlag( Tile::tlRoad ) )
        engine.draw( _d.greenTile , offset + tile->mappos() );
    }
  }

  if( opts.isFlag( DrawOptions::showWalkableTiles ) )
  {
    const TilesArray& tiles = _d.camera->tiles();
    for( auto tile : tiles )
    {
      if( tile->isWalkable( true ) )
        engine.draw( _d.greenTile, offset + tile->mappos() );
    }
  }

  if( opts.isFlag( DrawOptions::showFlatTiles ) )
  {
    const TilesArray& tiles = _d.camera->tiles();
    for( auto tile : tiles )
    {
      if( tile->isFlat() )
        engine.draw( _d.greenTile, offset + tile->mappos() );
    }
  }

  if( opts.isFlag( DrawOptions::showLockedTiles ) )
  {
    const TilesArray& tiles = _d.camera->tiles();
    for( auto tile : tiles )
    {
      if( !tile->isWalkable( true ) )
        engine.draw( _d.greenTile, offset + tile->mappos() );
    }
  }

  if( _d.currentTile && opts.isFlag( DrawOptions::showObjectArea ) )
  {
    Tile* tile = _d.currentTile;
    Point pos = tile->mappos();
    int rwidth = config::tilemap.cell.picSize().width();
    int halfRWidth = rwidth / 2;
    Size size( math::clamp<int>( (tile->picture().width() + 2) / rwidth, 1, 10 ) );

    if( _d.tilePosText.isValid() )
    {
      _d.tilePosText.fill( 0x0 );
      _d.debugFont.draw( _d.tilePosText, fmt::format( "{},{}", tile->i(), tile->j() ), false, true );
    }

    OverlayPtr ov = tile->overlay();
    if( ov.isValid() )
    {
      size = ov->size();
      pos = ov->tile().mappos();
    }
    else if( tile->master() != 0 )
    {
      pos = tile->master()->mappos();
      size = Size( (tile->master()->picture().width() + 2) / rwidth );
    }

    pos += offset;
    engine.drawLine( ColorList::red, pos, pos + Point( halfRWidth, halfRWidth/2 ) * size.height() );
    engine.drawLine( ColorList::red, pos + Point( halfRWidth, halfRWidth/2 ) * size.width(), pos + Point( rwidth, 0) * size.height() );
    engine.drawLine( ColorList::red, pos + Point( rwidth, 0) * size.width(), pos + Point( halfRWidth, -halfRWidth/2 ) * size.height() );
    engine.drawLine( ColorList::red, pos + Point( halfRWidth, -halfRWidth/2 ) * size.width(), pos );

    static int t=0;
    int a = (t++ % 40)/5;
    engine.drawLine( ColorList::red, pos - Point( a,0), pos + Point( halfRWidth, halfRWidth/2 ) * size.height() + Point( 0,a/2) );
    engine.drawLine( ColorList::red, pos + Point( halfRWidth, halfRWidth/2 ) * size.width() + Point( 0,a/2), pos + Point( rwidth, 0) * size.height() + Point(a, 0) );
    engine.drawLine( ColorList::red, pos + Point( rwidth, 0) * size.width() + Point(a,0), pos + Point( halfRWidth, -halfRWidth/2 ) * size.height() - Point(0,a/2));
    engine.drawLine( ColorList::red, pos + Point( halfRWidth, -halfRWidth/2 ) * size.width() - Point(0,a/2), pos - Point(a,0) );

#ifdef DEBUG
    engine.draw( _d.tilePosText, pos );
#endif
  }

  for( auto& pic : _d.pictures )
  {
    engine.draw( pic.pic, offset + pic.pos );
  }
}

Layer::Layer(Camera* camera, PlayerCityPtr city )
  : __INIT_IMPL(Layer)
{
  __D_REF(_d,Layer)
  _d.camera = camera;
  _d.greenTile = Picture( SETTINGS_STR(forbidenTile), 1 );
  _d.city = city;
  _d.debugFont = Font::create( FONT_1_WHITE );
  _d.currentTile = 0;
  _d.posMode = 0;
  _d.terraintPic = object::Info::find( object::terrain ).randomPicture( Size( 1 ) );
  _d.tilePosText = Picture( Size( 240, 80 ), 0, true );

  if( OSystem::isAndroid() )
    DrawOptions::instance().setFlag( DrawOptions::showObjectArea, true );
}

void Layer::_addWalkerType(walker::Type wtype)
{
  _dfunc()->visible.walkers.insert( wtype );
}

void Layer::_initialize()
{
  const VariantMap& vm = citylayer::Helper::getConfig( (citylayer::Type)type() );
  StringArray visibleObjectTypes = vm.get( "visibleObjects" ).toStringArray();
  for( const auto& it : visibleObjectTypes )
  {
    object::Type ovType = object::findType( it );
    if( ovType != object::unknown )
      _dfunc()->visible.objects.insert( ovType );
  }
}

bool Layer::_moveCamera(NEvent &event)
{
  __D_REF(_d,Layer)
  bool pressed = event.keyboard.pressed;
  int moveValue = math::clamp<int>( _d.camera->scrollSpeed()/10, 1, 99 );

  moveValue *= ( event.keyboard.shift ? 4 : 1 ) * (pressed ? 1 : 0);

  switch( event.keyboard.key )
  {
  case KEY_UP:    _d.camera->moveUp   ( moveValue ); break;
  case KEY_DOWN:  _d.camera->moveDown ( moveValue ); break;
  case KEY_RIGHT: _d.camera->moveRight( moveValue ); break;
  case KEY_LEFT:  _d.camera->moveLeft ( moveValue ); break;
  default:
      return false;
  }

  return true;
}

Tilemap& Layer::_map() const
{
  __D_REF(_d,Layer)
  if( _d.city.isValid() )
    return _d.city->tilemap();

  Logger::warning( "!!! WARNING: City is null at Walker::_map()" );
  return config::tilemap.invalid();
}

bool Layer::_isVisibleObject(object::Type ovType)
{
  auto& objects = _dfunc()->visible.objects;
  if( objects.empty() )
    return true;
  else
    return objects.count( ovType ) > 0;
}

Layer::WalkerTypes& Layer::_visibleWalkers() { return _dfunc()->visible.walkers; }
int Layer::nextLayer() const{ return _dfunc()->nextLayer; }
void Layer::destroy() {}

void Layer::addDrawPass(int type, LayerDrawPassPtr pass) { _dfunc()->passes[ type ].push_back(pass); }
Camera* Layer::_camera(){ return _dfunc()->camera; }
PlayerCityPtr Layer::_city(){ return _dfunc()->city; }
void Layer::changeLayer(int type) {}
void Layer::_setNextLayer(int layer) { _dfunc()->nextLayer = layer;}
Layer::~Layer(){}
void Layer::_setLastCursorPos(Point pos){ _dfunc()->cursor.last = pos; }
void Layer::_setStartCursorPos(Point pos){ _dfunc()->cursor.start = pos; }
Point Layer::_startCursorPos() const{ return _dfunc()->cursor.start; }
Tile* Layer::_currentTile() const{ return _dfunc()->currentTile; }
Point Layer::_lastCursorPos() const { return _dfunc()->cursor.last; }

bool Layer::_isMovingButtonPressed(NEvent &event) const
{
  return DrawOptions::instance().isFlag( DrawOptions::mmbMoving )
            ? event.mouse.isMiddlePressed()
            : event.mouse.isLeftPressed();
}

DrawOptions& DrawOptions::instance()
{
  static DrawOptions inst;
  return inst;
}

bool DrawOptions::getFlag(DrawOptions::Flag flag)
{
  return instance().isFlag( flag );
}

void DrawOptions::takeFlag(DrawOptions::Flag flag, int value)
{
  instance().setFlag( flag, value );
}

DrawOptions::Flag DrawOptions::findFlag(const std::string& name)
{
  return (Flag)instance()._helper.findType( name );
}

DrawOptions::DrawOptions() : _helper(0)
{
#define _O(a) _helper.append( DrawOptions::a, TEXT(a) );
  _O(drawGrid)
  _O(shadowOverlay)
  _O(showPath)
  _O(windowActive)
  _O(showRoads)
  _O(showObjectArea)
  _O(showWalkableTiles)
  _O(showLockedTiles)
  _O(showFlatTiles)
  _O(borderMoving)
  _O(mayChangeLayer)
  _O(oldGraphics)
  _O(mmbMoving)
  _O(showBuildings)
  _O(showTrees)
  _O(overdrawOnBuild)
  _O(rotateEnabled)
#undef _O
}

}//end namespace gfx
