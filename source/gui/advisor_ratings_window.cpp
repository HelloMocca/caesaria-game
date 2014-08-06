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

#include "advisor_ratings_window.hpp"
#include "gfx/picture.hpp"
#include "gfx/decorator.hpp"
#include "core/gettext.hpp"
#include "pushbutton.hpp"
#include "label.hpp"
#include "game/resourcegroup.hpp"
#include "core/stringhelper.hpp"
#include "gfx/engine.hpp"
#include "core/font.hpp"
#include "objects/construction.hpp"
#include "city/city.hpp"
#include "city/victoryconditions.hpp"
#include "texturedbutton.hpp"
#include "city/cityservice_culture.hpp"
#include "city/cityservice_prosperity.hpp"
#include "core/logger.hpp"

using namespace gfx;
using namespace city;

namespace gui
{

namespace advisorwnd
{

class RatingButton : public PushButton
{
public:
  RatingButton( Widget* parent, Point pos, std::string title, std::string tooltip )
    : PushButton( parent, Rect( pos, Size( 108, 65 )), _(title), -1, false, PushButton::whiteBorderUp )
  {
    setTextAlignment( align::center, align::upperLeft );
    setTooltipText( _(tooltip) );
    _value = 0;
    _target = 0;
  }

  virtual void _updateTextPic()
  {
    PushButton::_updateTextPic();

    Font digitFont = Font::create( FONT_4 );
    PictureRef& pic = _textPictureRef();
    if( pic )
    {
      digitFont.draw( *pic, StringHelper::format( 0xff, "%d", _value ), width() / 2 - 10, 17, true, false );

      Font targetFont = Font::create( FONT_1 );
      targetFont.draw( *pic, StringHelper::format( 0xff, "%d %s", _target, _("##wndrt_need##") ), 10, height() - 20, true, false );

      pic->update();
    }     
  }

  void setValue( const int value )
  {
    _value = value;
    _resizeEvent();
  }

  void setTarget( const int value )
  {
    _target = value;
    _resizeEvent();
  }

private:
  int _value;
  int _target;
};

class Ratings::Impl
{
public:
  Pictures columns;
  RatingButton* btnCulture;
  RatingButton* btnProsperity;
  RatingButton* btnPeace;
  RatingButton* btnFavour;
  TexturedButton* btnHelp;
  Label* lbRatingInfo;

  void updateColumn( const Point& alignCenter, const int value );
  void checkCultureRating();
  void checkProsperityRating();
  void checkPeaceRating();

  PlayerCityPtr city;
};

void Ratings::Impl::updateColumn( const Point& center, const int value )
{
  int columnStartY = 275;
  const Picture& footer = Picture::load( ResourceGroup::panelBackground, 544 );
  const Picture& header = Picture::load( ResourceGroup::panelBackground, 546 );
  const Picture& body = Picture::load( ResourceGroup::panelBackground, 545 );

  for( int i=0; i < value; i++ )
  {
    columns.append( body, Point( center.x() - body.width() / 2, -columnStartY + (10 + i * 2)) );
  }

  columns.append( footer, Point( center.x() - footer.width() / 2, -columnStartY + footer.height()) );
  if( value >= 50 )
  {
    columns.append( header, Point( center.x() - header.width() / 2, -columnStartY + (10 + value * 2)) );
  }
}

void Ratings::Impl::checkCultureRating()
{
  SmartPtr< city::CultureRating > culture = ptr_cast<city::CultureRating>( city->findService( city::CultureRating::defaultName() ) );

  if( culture != 0 )
  {
    if( culture->value() == 0 )
    {
      lbRatingInfo->setText( _("##no_culture_building_in_city##") );
      return;
    }

    StringArray troubles;

    const char* covTypename[CultureRating::covCount] = { "school", "library", "academy", "temple", "theater" };
    for( int k=CultureRating::covSchool; k < CultureRating::covCount; k++)
    {
      int coverage = culture->coverage( CultureRating::Coverage(k) );
      if( coverage < 100 )
      {
        std::string troubleDesc = StringHelper::format( 0xff, "##have_less_%s_in_city_%d##", covTypename[ k ], coverage / 50 );
        troubles.push_back( troubleDesc );
      }
    }

    if( !troubles.empty() )
    {
      lbRatingInfo->setText( _( troubles.random() ) );
    }
  }
}

void Ratings::Impl::checkProsperityRating()
{
  SmartPtr< city::ProsperityRating > prosperity = ptr_cast<city::ProsperityRating>( city->findService( city::ProsperityRating::defaultName() ) );

  if( prosperity != 0 )
  {
    if( prosperity->value() == 0 )
    {
      lbRatingInfo->setText( _("##cant_calc_prosperity##") );
      return;
    }

    StringArray troubles;
    if( prosperity->getMark( city::ProsperityRating::cmHousesCap ) < 0 ) { troubles.push_back( _("##bad_house_quality##") ); }
    if( prosperity->getMark( city::ProsperityRating::cmHaveProfit ) == 0 ) { troubles.push_back( _("##lost_money_last_year##") ); }
    if( prosperity->getMark( city::ProsperityRating::cmWorkless ) > 15 ) { troubles.push_back( _("##high_workless_number##") ); }
    if( prosperity->getMark( city::ProsperityRating::cmWorkersSalary ) < 0 ) { troubles.push_back( _("##workers_salary_less_then_rome##") ); }
    if( prosperity->getMark( city::ProsperityRating::cmPercentPlebs ) > 30 ) { troubles.push_back( _("##much_plebs##") ); }
    if( prosperity->getMark( city::ProsperityRating::cmChange ) == 0 )
    {
      troubles.push_back( _("##no_prosperity_change##") );
      troubles.push_back( _("##how_to_grow_prosperity##") );
    }

    std::string text = troubles.empty()
                        ? _("##good_prosperity##")
                        : troubles[ (int)(rand() % troubles.size()) ];

    lbRatingInfo->setText( text );
  }
}

void Ratings::Impl::checkPeaceRating()
{
  lbRatingInfo->setText( _("##peace_rating_text##") );
}

Ratings::Ratings(Widget* parent, int id, const PlayerCityPtr city )
  : Window( parent, Rect( 0, 0, 640, 432 ), "", id ), _d( new Impl )
{
  _d->city = city;
  setupUI( ":/gui/ratingsadv.gui" );
  setPosition( Point( (parent->width() - 640 )/2, parent->height() / 2 - 242 ) );

  //buttons _d->_d->background
  Rect r( Point( 66, 360 ), Size( 510, 60 ) );
  _d->lbRatingInfo = findChildA<Label*>( "lbRatingInfo", true, this );

  const city::VictoryConditions& targets = city->victoryConditions();

  Label* lbNeedPopulation = findChildA<Label*>( "lbNeedPopulation", true, this );
  if( lbNeedPopulation ) lbNeedPopulation->setText( StringHelper::format( 0xff, "(%s %d)", _("##need_population##"), targets.needPopulation() ) );

  _d->btnCulture    = new RatingButton( this, Point( 80,  290), "##wdnrt_culture##", "##wndrt_culture_tooltip##" );
  _d->btnCulture->setTarget( targets.needCulture() );
  _d->btnCulture->setValue( _d->city->culture() );
  _d->updateColumn( _d->btnCulture->relativeRect().getCenter(), 0 );
  CONNECT( _d->btnCulture, onClicked(), _d.data(), Impl::checkCultureRating );

  _d->btnProsperity = new RatingButton( this, Point( 200, 290), "##wndrt_prosperity##", "##wndrt_prosperity_tooltip##" );
  _d->btnProsperity->setValue( _d->city->prosperity() );
  _d->btnProsperity->setTarget( targets.needProsperity() );
  _d->updateColumn( _d->btnProsperity->relativeRect().getCenter(), _d->city->prosperity() );
  CONNECT( _d->btnProsperity, onClicked(), _d.data(), Impl::checkProsperityRating );

  _d->btnPeace      = new RatingButton( this, Point( 320, 290), "##wndrt_peace##", "##wndrt_peace_tooltip##" );
  _d->btnPeace->setValue( _d->city->peace() );
  _d->btnPeace->setTarget( targets.needPeace() );
  _d->updateColumn( _d->btnPeace->relativeRect().getCenter(), 0 );
  CONNECT( _d->btnPeace, onClicked(), _d.data(), Impl::checkPeaceRating );

  _d->btnFavour     = new RatingButton( this, Point( 440, 290), "##wndrt_favour##", "##wndrt_favour_tooltip##" );
  _d->btnFavour->setValue( _d->city->favour() );
  _d->btnFavour->setTarget( targets.needFavour() );
  _d->updateColumn( _d->btnFavour->relativeRect().getCenter(), 0 );

  _d->btnHelp = new TexturedButton( this, Point( 12, height() - 39), Size( 24 ), -1, ResourceMenu::helpInfBtnPicId );
}

void Ratings::draw( gfx::Engine& painter )
{
  if( !visible() )
    return;

  Window::draw( painter );

  painter.draw( _d->columns, absoluteRect().lefttop(), &absoluteClippingRectRef() );
}

}

}//end namespace gui
