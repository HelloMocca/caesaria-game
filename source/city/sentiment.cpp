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
// Copyright 2012-2013 Dalerank, dalerankn8@gmail.com

#include "sentiment.hpp"
#include "game/gamedate.hpp"
#include "objects/house.hpp"
#include "core/variant_map.hpp"
#include "core/gettext.hpp"
#include "config.hpp"
#include "city/statistic.hpp"
#include "cityservice_factory.hpp"

using namespace config;

namespace city
{

REGISTER_SERVICE_IN_FACTORY(Sentiment,sentiment)

struct BuffInfo
{
  int value;
  int finishValue;
  bool relative;
  DateTime finishDate;

  BuffInfo()
  {
    value = 0;
    finishValue = 0;
    relative = false;
  }

  VariantMap save() const
  {
    VariantMap ret;
    VARIANT_SAVE_ANY( ret, value )
    VARIANT_SAVE_ANY( ret, relative )
    VARIANT_SAVE_ANY( ret, finishDate )
    VARIANT_SAVE_ANY( ret, finishValue )

    return ret;
  }

  void load( const VariantMap& stream )
  {
    VARIANT_LOAD_ANY( value, stream )
    VARIANT_LOAD_ANY( relative, stream )
    VARIANT_LOAD_TIME( finishDate, stream )
    VARIANT_LOAD_ANY( finishValue, stream )
  }
};

class Buffs : public std::vector<BuffInfo>
{
public:
  VariantList save() const
  {
    VariantList vlBuffs;
    for( auto& item : *this )
      vlBuffs.push_back( item.save() );

    return vlBuffs;
  }

  void load( const VariantList& vl )
  {
    for( auto& item : vl )
    {
      BuffInfo buff;
      buff.load( item.toMap() );
      push_back( buff );
    }
  }
};

class Sentiment::Impl
{
public:  
  struct {
    int current;
    int finish;
  } value;

  int affect;
  int buffValue;
  Buffs buffs;
};

std::string Sentiment ::defaultName() { return TEXT(Sentiment);}

Sentiment::Sentiment( PlayerCityPtr city )
  : Srvc( city, defaultName() ), _d( new Impl )
{
  _d->value.current = defaultValue;
  _d->value.finish = defaultValue;
  _d->affect = 0;
  _d->buffValue = 0;
}

void Sentiment::timeStep(const unsigned int time )
{
  if( game::Date::isWeekChanged() )
  {
    _d->value.current += math::signnum( _d->value.finish - _d->value.current );
  }

  if( game::Date::isMonthChanged() )
  {
    _d->buffValue = 0;
    DateTime current = game::Date::current();
    for( Buffs::iterator it=_d->buffs.begin(); it != _d->buffs.end(); )
    {
      BuffInfo& buff = *it;
      if( buff.finishDate > current )
      {
        it = _d->buffs.erase( it );
      }
      else
      {
        if( buff.relative ) { buff.finishValue += buff.value; }
        else { buff.finishValue = buff.value; }

        _d->buffValue += buff.value;
        ++it;
      }
    }

    HouseList houses = _city()->statistic().houses.find();

    unsigned int houseNumber = 0;
    _d->value.finish = 0;
    for( auto house : houses )
    {
      house->setState( pr::happinessBuff, _d->buffValue );

      if( house->habitants().count() > 0 )
      {
        _d->value.finish += house->state( pr::happiness );
        houseNumber++;
      }
    }

    if( houseNumber > 0 )
      _d->value.finish /= houseNumber;
    else
      _d->value.finish = defaultValue;
  }
}

int Sentiment::value() const { return _d->value.current + _d->affect; }
int Sentiment::buff() const { return _d->buffValue; }

std::string Sentiment::reason() const
{
  std::string ret = "##unknown_sentiment_reason##";
  int v = value();
  if( v > sentiment::idolizeyou )            { ret = "##sentiment_people_idolize_you##"; }
  else if( v > sentiment::loveYou )          { ret = "##sentiment_people_love_you##"; }
  else if( v > sentiment::extrimelyPleased ) { ret = "##sentiment_people_extr_pleased_you##"; }
  else if( v > sentiment::veryPleased)       { ret = "##sentiment_people_verypleased_you##"; }
  else if( v > sentiment::pleased )          { ret = "##sentiment_people_pleased_you##"; }
  else if( v > sentiment::indifferent )      { ret = "##sentiment_people_indiffirent_you##"; }
  else if( v > sentiment::annoyed )          { ret = "##sentiment_people_annoyed_you##"; }
  else if( v > sentiment::upset )            { ret = "##sentiment_people_upset_you##"; }
  else if( v > sentiment::veryUpset )        { ret = "##sentiment_people_veryupset_you##"; }
  else if( v > sentiment::angry )            { ret = "##sentiment_people_angry_you##"; }
  else if( v > sentiment::veryAngry )        { ret = "##sentiment_people_veryangry_you##"; }
  else if( v > 0 )                           { ret = "##city_loathed_you##"; }

  return _(ret);
}

VariantMap Sentiment::save() const
{
  VariantMap ret = Srvc::save();
  VARIANT_SAVE_ANY_D( ret, _d, value.current )
  VARIANT_SAVE_ANY_D( ret, _d, value.finish )
  VARIANT_SAVE_ANY_D( ret, _d, affect )
  VARIANT_SAVE_CLASS_D( ret, _d, buffs )
  return ret;
}

void Sentiment::load(const VariantMap& stream)
{
  Srvc::load( stream );
  VARIANT_LOAD_ANY_D( _d, value.current, stream )
  VARIANT_LOAD_ANY_D( _d, value.finish, stream )
  VARIANT_LOAD_ANY_D( _d, affect, stream )
  VARIANT_LOAD_CLASS_D_LIST( _d, buffs, stream )
}

void Sentiment::addBuff(int value, bool relative, int month2finish)
{
  BuffInfo buff;
  buff.value = value;
  buff.relative = relative;
  buff.finishDate = game::Date::current();
  buff.finishDate.appendMonth( month2finish );

  _d->buffs.push_back( buff );
}

}//end namespace city
