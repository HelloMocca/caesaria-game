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

#include "cityservice_emigrant.hpp"
#include "city.hpp"
#include "core/safetycast.hpp"
#include "gfx/tilemap.hpp"
#include "walker/emigrant.hpp"
#include "core/position.hpp"
#include "road.hpp"
#include "building/house.hpp"
#include "gfx/tile.hpp"
#include "cityfunds.hpp"
#include "building/constants.hpp"
#include "settings.hpp"
#include "world/empire.hpp"

using namespace constants;

class CityServiceEmigrant::Impl
{
public:
  PlayerCityPtr city;
};

CityServicePtr CityServiceEmigrant::create(PlayerCityPtr city )
{
  CityServicePtr ret( new CityServiceEmigrant( city ) );
  ret->drop();

  return ret;
}

CityServiceEmigrant::CityServiceEmigrant( PlayerCityPtr city )
: CityService( "emigration" ), _d( new Impl )
{
  _d->city = city;
}

void CityServiceEmigrant::update( const unsigned int time )
{
  if( time % 44 != 1 )
    return;
  
  unsigned int vacantPop=0;
  int emigrantsIndesirability = 50; //base indesirability value
  float emDesKoeff = math::clamp<float>( (float)GameSettings::get( GameSettings::emigrantSalaryKoeff ), 1.f, 99.f );
  //if salary in city more then empire people more effectivelly go to ouu city
  emigrantsIndesirability += (_d->city->getEmpire()->getWorkersSalary() - _d->city->getFunds().getWorkerSalary()) * emDesKoeff;

  int worklessPercent = CityStatistic::getWorklessPercent( _d->city );
  emigrantsIndesirability += worklessPercent == 0
                            ? -10
                            : (worklessPercent * (worklessPercent < 15 ? 1 : 2));

  int goddesRandom = rand() % 100;
  if( goddesRandom < emigrantsIndesirability )
    return;

  CityHelper helper( _d->city );
  HouseList houses = helper.find<House>(building::house);
  foreach( HousePtr house, houses )
  {
    if( house->getAccessRoads().size() > 0 )
    {
      vacantPop += math::clamp( house->getMaxHabitants() - house->getHabitants().count(), 0, 0xff );
    }
  }

  if( vacantPop == 0 )
  {
    return;
  }

  WalkerList walkers = _d->city->getWalkers( walker::emigrant );

  if( vacantPop <= walkers.size() * 5 )
  {
    return;
  }

  Tile& roadTile = _d->city->getTilemap().at( _d->city->getBorderInfo().roadEntry );

  EmigrantPtr emigrant = Emigrant::create( _d->city );

  if( emigrant.isValid() )
  {
    emigrant->send2City( roadTile );
  }
}
