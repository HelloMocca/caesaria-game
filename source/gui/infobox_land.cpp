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

#include "infobox_land.hpp"
#include "label.hpp"
#include "core/gettext.hpp"
#include "city/city.hpp"
#include "environment.hpp"
#include "objects/road.hpp"
#include "core/utils.hpp"
#include "objects/constants.hpp"
#include "pathway/pathway_helper.hpp"
#include "dictionary.hpp"
#include "game/infoboxmanager.hpp"

using namespace gfx;

namespace gui
{

namespace infobox
{

REGISTER_OBJECT_BASEINFOBOX(tree,AboutLand)

AboutLand::AboutLand(Widget* parent, PlayerCityPtr city, const Tile& tile )
  : Infobox( parent, Rect( 0, 0, 510, 350 ), Rect( 16, 60, 510 - 16, 60 + 180) )
{ 
  int id = lbTextId;
  Label& lbText = add<Label>( Rect( 38, 60, 470, 60+180 ), "", true, Label::bgNone, id );
  lbText.setFont( Font::create( FONT_2 ) );
  lbText.setTextAlignment( align::upperLeft, align::center );
  lbText.setWordwrap( true );

  std::string text;
  std::string title;

  if( tile.pos() == city->getBorderInfo( PlayerCity::roadExit ).epos() )
  {
    title = "##to_empire_road##";
    _helpUri = "road_to_empire";
    text = "";
  }
  else if( tile.pos() == city->getBorderInfo( PlayerCity::roadEntry ).epos() )
  {
    title = "##to_rome_road##";
    text = "";
  }
  else if( tile.getFlag( Tile::tlTree ) )
  {
    title = "##trees_and_forest_caption##";
    _helpUri = "trees";
    text = "##trees_and_forest_text##";    
  } 
  else if( tile.getFlag( Tile::tlWater ) )
  {
    std::string typeStr = tile.getFlag( Tile::tlCoast )
                            ? "##coast"
                            : "##water";
    title = typeStr + "_caption##";

    TilePos exitPos = city->getBorderInfo( PlayerCity::boatEntry ).epos();
    Pathway way = PathwayHelper::create( tile.pos(), exitPos, PathwayHelper::deepWaterFirst );

    text = way.isValid()
             ? (typeStr + "_text##")
             : "##inland_lake_text##";
    _helpUri = "water";
  }
  else if( tile.getFlag( Tile::tlRock ) )
  {
    title = "##rock_caption##";
    _helpUri = "rock";
    text = "##rock_text##";
  }
  else if( tile.getFlag( Tile::tlRoad ) )
  {
    object::Type ovType = object::typeOrDefault( tile.overlay() );
    if(ovType == object::plaza )
    {
      title = "##plaza_caption##";
      _helpUri = "plaza";
      text = "##plaza_text##";
    }
    else if( ovType == object::road )
    {
      _helpUri = "paved_road";
      auto road = tile.overlay<Road>();
      title = road->pavedValue() > 0 ? "##road_paved_caption##" : "##road_caption##";
      if( tile.pos() == city->getBorderInfo( PlayerCity::roadEntry ).epos() ) { text = "##road_from_rome##"; }
      else if( tile.pos() == city->getBorderInfo( PlayerCity::roadExit ).epos() ) { text = "##road_to_distant_region##"; }
      else text = road->pavedValue() > 0 ? "##road_paved_text##" : "##road_text##";
    }
    else
    {
      title = "##road_caption##";
      _helpUri = "road";
      text = "##road_unknown_text##";
    }
  }
  else if( tile.getFlag( Tile::tlMeadow ) )
  {
    title = "##meadow_caption##";
    _helpUri = "meadow";
    text = "##meadow_text##";
  }
  else 
  {
    title = "##clear_land_caption##";
    _helpUri = "clear_land";
    text = "##clear_land_text##";
  }
  
  //int index = (size - tile.getJ() - 1 + border_size) * 162 + tile.getI() + border_size;

  text = _(text );
#ifdef DEBUG
  text += utils::format( 0xff, "\nTile at: (%d,%d) ID:%04X",
                                           tile.i(), tile.j(),  
                                          ((unsigned int) tile.imgId() ) );
#endif
  setTitle( _( title ));
  setText( text );
}

void AboutLand::setText( const std::string& text )
{
  if( Widget* lb = findChild( lbTextId ) )
    lb->setText( text );
}

void AboutLand::_showHelp() { ui()->add<DictionaryWindow>( _helpUri ); }
void AboutFreeHouse::_showHelp() { ui()->add<DictionaryWindow>( "vacant_lot" ); }

AboutFreeHouse::AboutFreeHouse( Widget* parent, PlayerCityPtr city, const Tile& tile )
    : AboutLand( parent, city, tile )
{
  setTitle( _("##freehouse_caption##") );

  auto cnst = tile.overlay<Construction>();
  if( cnst.isValid() )
  {
      setText( cnst->roadside().empty()
                  ? _("##freehouse_text_noroad##")
                  : _("##freehouse_text##") );
  }
}


}//end namespace infobox

}//end namespace gui
