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

#include "wrath_of_venus.hpp"
#include "game/game.hpp"
#include "objects/construction.hpp"
#include "city.hpp"
#include "core/variant_map.hpp"
#include "game/gamedate.hpp"
#include "objects/house.hpp"
#include "walker/enemysoldier.hpp"
#include "core/logger.hpp"
#include "cityservice_factory.hpp"
#include "events/dispatcher.hpp"
#include "statistic.hpp"

namespace city
{

REGISTER_SERVICE_IN_FACTORY(WrathOfVenus,wrathOfVenus)

class WrathOfVenus::Impl
{
public:
  DateTime endTime;
  int strong;
  bool isDeleted;
};

void WrathOfVenus::timeStep( const unsigned int time)
{
  if( game::Date::isDayChanged() )
  {
    _d->isDeleted = (_d->endTime < game::Date::current());

    Logger::warning( "WrathOfVenus: execute service" );

    auto citizens = _city()->statistic().walkers.find<Human>();
    citizens = citizens.random( _d->strong );

    for( auto wlk : citizens )
      wlk->die();
  }
}

std::string WrathOfVenus::defaultName() { return "wrath_of_venus"; }
bool WrathOfVenus::isDeleted() const {  return _d->isDeleted; }

void WrathOfVenus::load(const VariantMap& stream)
{
  VARIANT_LOAD_TIME_D( _d, endTime, stream )
  VARIANT_LOAD_ANY_D( _d, isDeleted, stream )
  VARIANT_LOAD_ANY_D( _d, strong, stream )
}

VariantMap WrathOfVenus::save() const
{
  VariantMap ret = Srvc::save();
  VARIANT_SAVE_ANY_D( ret, _d, endTime )
  VARIANT_SAVE_ANY_D( ret, _d, isDeleted )
  VARIANT_SAVE_ANY_D( ret, _d, strong )

  return ret;
}

WrathOfVenus::WrathOfVenus(PlayerCityPtr city, int month, int strong )
  : Srvc( city, WrathOfVenus::defaultName() ), _d( new Impl )
{
  _d->isDeleted = false;
  _d->endTime = game::Date::current();
  _d->endTime.appendMonth( month );
  _d->strong = strong;
  _d->strong = 1;
}

}//end namespace city
