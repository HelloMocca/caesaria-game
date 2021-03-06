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

#include <cstdio>

#include "infobox_factory.hpp"
#include "core/utils.hpp"
#include "core/gettext.hpp"
#include "label.hpp"
#include "image.hpp"
#include "good/stock.hpp"
#include "good/helper.hpp"
#include "dictionary.hpp"
#include "environment.hpp"
#include "objects/shipyard.hpp"
#include "objects/wharf.hpp"
#include "core/logger.hpp"
#include "widget_helper.hpp"
#include "city/city.hpp"
#include "game/infoboxmanager.hpp"

using namespace gfx;

namespace gui
{

namespace infobox
{

REGISTER_OBJECT_BASEINFOBOX(shipyard,AboutShipyard)
REGISTER_OBJECT_BASEINFOBOX(wharf,AboutWharf)
REGISTER_OBJECT_BASEINFOBOX(pottery_workshop,AboutFactory)
REGISTER_OBJECT_BASEINFOBOX(weapons_workshop,AboutFactory)
REGISTER_OBJECT_BASEINFOBOX(furniture_workshop,AboutFactory)
REGISTER_OBJECT_BASEINFOBOX(wine_workshop,AboutFactory)
REGISTER_OBJECT_BASEINFOBOX(oil_workshop,AboutFactory)

AboutFactory::AboutFactory(Widget* parent, PlayerCityPtr city, const Tile& tile)
  : AboutConstruction( parent, Rect( 0, 0, 510, 256 ), Rect( 16, 160, 510 - 16, 160 + 52) )
{
  setupUI( ":/gui/infoboxfactory.gui" );

  auto factory = tile.overlay<Factory>();

  if( factory.isNull() )
  {
    deleteLater();
    return;
  }

  setBase( factory );
  _type = factory->type();

  setTitle( _( factory->info().prettyName() ) );
  _setWorkingVisible( true );

  if( !factory.isValid() )
  {
    Logger::warning( "AboutFactory: cant convert base to factory at [{0},{1}]", tile.i(), tile.j() );
    deleteLater();
    return;
  }

  // paint progress
  std::string text = utils::format( 0xff, "%s %d%%", _("##rawm_production_complete_m##"), factory->progress() );
  Size lbSize( (width() - 20) / 2, 25 );
  _lbProduction = &add<Label>( Rect( _lbTitle()->leftbottom() + Point( 10, 0 ), lbSize ), text );
  _lbProduction->setFont( Font::create( FONT_2 ) );

  std::string effciencyText = utils::format( 0xff, "%s %d%%", _("##effciency##"), factory->effciency() );
  _lbEffciency = &add<Label>( _lbProduction->relativeRect() + Point( lbSize.width(), 0 ), effciencyText );
  _lbEffciency->setFont( Font::create( FONT_2 ) );


  if( factory->produceGoodType() != good::none )
  {
    add<Image>( Point( 10, 10), good::Helper::picture( factory->produceGoodType() ) );
  }

  // paint picture of in good
  if( factory->inStock().type() != good::none )
  {
    Label& lbStockInfo = add<Label>( Rect( _lbTitle()->leftbottom() + Point( 0, 25 ), Size( width() - 32, 25 ) ) );
    lbStockInfo.setIcon( good::Helper::picture( factory->inStock().type() ) );

    std::string whatStock = fmt::format( "##{0}_factory_stock##", good::Helper::getTypeName( factory->consumeGoodType() ) );
    std::string typeOut = fmt::format( "##{0}_factory_stock##", good::Helper::getTypeName( factory->produceGoodType() ) );
    std::string text = utils::format( 0xff, "%d %s %d %s",
                                      factory->inStock().qty() / 100,
                                      _(whatStock),
                                      factory->outStock().qty() / 100,
                                      _(typeOut) );

    lbStockInfo.setText( text );
    lbStockInfo.setTextOffset( Point( 30, 0 ) );

    _lbText()->setGeometry( Rect( lbStockInfo.leftbottom() + Point( 0, 5 ),
                                  _lbBlackFrame()->righttop() - Point( 0, 5 ) ) );
    _lbText()->setFont( Font::create( FONT_1 ) );
  }

  std::string workInfo = factory->workersProblemDesc();
  std::string cartInfo = factory->cartStateDesc();
  setText( utils::format( 0xff, "%s %s", _(workInfo), _( cartInfo ) ) );

  _updateWorkersLabel( Point( 32, 157 ), 542, factory->maximumWorkers(), factory->numberWorkers() );
}

void AboutFactory::_showHelp() {  ui()->add<DictionaryWindow>( _type ); }

AboutShipyard::AboutShipyard(Widget* parent, PlayerCityPtr city, const Tile& tile)
  : AboutFactory( parent, city, tile )
{
  auto shipyard = tile.overlay<Shipyard>();

  int progressCount = shipyard->progress();
  if( progressCount > 1 && progressCount < 100 )
  {
    Label& lb = add<Label>( Rect( _lbProduction->leftbottom() + Point( 0, 5 ), Size( width() - 90, 25 ) ),
                            _("##build_fishing_boat##") );
    lb.setTextAlignment( align::upperLeft, align::upperLeft );
    _lbText()->setPosition( lb.leftbottom() + Point( 0, 5 ) );
  }
}


AboutWharf::AboutWharf(Widget* parent, PlayerCityPtr city, const Tile& tile)
  : AboutFactory( parent, city, tile )
{
  auto wharf = tile.overlay<Wharf>();

  if( wharf->getBoat().isNull() )
  {
    Label& lb = add<Label>( Rect( _lbProduction->leftbottom() + Point( 0, 10 ), Size( width() - 90, 25 ) ),
                            _("##wait_for_fishing_boat##") );
    lb.setTextAlignment( align::upperLeft, align::upperLeft );
    lb.setWordwrap( true );
    _lbText()->setPosition( lb.leftbottom() + Point( 0, 10 ) );
  }
}

}

}//end namespace gui
