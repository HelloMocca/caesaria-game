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
// Copyright 2012-2014 dalerank, dalerankn8@gmail.com

#include "enemysoldier.hpp"
#include "core/position.hpp"
#include "pathway/astarpathfinding.hpp"
#include "pathway/path_finding.hpp"
#include "gfx/tile.hpp"
#include "gfx/tilemap.hpp"
#include "city/statistic.hpp"
#include "core/priorities.hpp"
#include "core/variant_map.hpp"
#include "name_generator.hpp"
#include "core/utils.hpp"
#include "events/event.hpp"
#include "core/logger.hpp"
#include "objects/construction.hpp"
#include "corpse.hpp"
#include "game/resourcegroup.hpp"
#include "pathway/pathway_helper.hpp"
#include "helper.hpp"
#include "animals.hpp"
#include "throwing_weapon.hpp"
#include "core/foreach.hpp"
#include "events/militarythreat.hpp"
#include "walkers_factory.hpp"
#include "objects/metadata.hpp"

using namespace gfx;

REGISTER_NAMED_CLASS_IN_WALKERFACTORY( walker::britonSoldier, EnemySoldier, briton )
REGISTER_NAMED_CLASS_IN_WALKERFACTORY( walker::gladiatorRiot, EnemySoldier, glriot)
REGISTER_NAMED_CLASS_IN_WALKERFACTORY( walker::etruscanSoldier, EnemySoldier, etruscan)

EnemySoldier::EnemySoldier( PlayerCityPtr city, walker::Type type )
  : Soldier( city, type )
{
  _setSubAction( check4attack );
  setAttackPriority( attackAll );
  setAttackDistance( 1 );

  _atExclude << object::group::disaster
             << object::group::road
             << object::group::garden;

  addFriend( type );
  _failedWayCounter = 0;
}

object::GroupSet& EnemySoldier::_excludeAttack() {  return _atExclude; }

bool EnemySoldier::_tryAttack()
{
  WalkerList enemies = _findEnemiesInRange( attackDistance() );

  if( !enemies.empty() )
  {
    _setSubAction( Soldier::fightEnemy );
    setTarget( enemies.front()->pos() );
    fight();
  }
  else
  {
    ConstructionList constructions = _findContructionsInRange( attackDistance() );
    if( !constructions.empty() )
    {
      _setSubAction( Soldier::destroyBuilding );
      setTarget( constructions.front()->pos() );
      fight();
    }
  }

  if( action() == acFight )
  {
    bool needMeMove = false;
    _city()->statistic().map.isTileBusy<EnemySoldier>( pos(), this, needMeMove );

    if( needMeMove )
    {
      _move2freePos( target() );
    }
  }

  return action() == acFight;
}

void EnemySoldier::_waitFinished()
{
  _setSubAction( check4attack );
}

EnemySoldier::AttackPriority EnemySoldier::_attackPriority() const { return _atPriority; }

void EnemySoldier::_brokePathway(TilePos pos)
{
  if( !_tryAttack() )
  {
    _check4attack();
  }
}

void EnemySoldier::_reachedPathway()
{
  Soldier::_reachedPathway();
  switch( _subAction() )
  {
  case check4attack:
  case go2enemy:
  case go2position:
  {
    bool findAny4attack = _tryAttack();
    if( !findAny4attack )
    {
      _check4attack();
    }
  }
  break;

  default: break;
  }
}

WalkerList EnemySoldier::_findEnemiesInRange( unsigned int range )
{
  Tilemap& tmap = _map();
  WalkerList walkers;

  for( unsigned int k=0; k <= range; k ++ )
  {
    TilesArray tiles = tmap.rect( k, pos() );

    walker::Type rtype;
    for( auto tile : tiles )
    {
      const WalkerList& tileWalkers = _city()->walkers( tile->pos() );

      for( auto wlk : tileWalkers )
      {
        rtype = wlk->type();
        if( rtype == type() || !WalkerHelper::isHuman( wlk ) || isFriendTo( wlk ) )
          continue;

        walkers.push_back( wlk );
      }
    }
  }

  return walkers;
}

Pathway EnemySoldier::_findPathway2NearestEnemy( unsigned int range )
{
  Pathway ret;

  WalkerList walkers = _findEnemiesInRange( range );

  for( auto wlk : walkers)
  {
    ret = PathwayHelper::create( pos(), wlk->pos(), PathwayHelper::allTerrain );
    if( ret.isValid() )
    {
      return ret;
    }
  }

  return Pathway();
}

void EnemySoldier::_check4attack()
{
  //try find any walkers in range
  Pathway pathway = _findPathway2NearestEnemy( 10 );

  if( !pathway.isValid() )
  {
    int size = _map().size();
    pathway = _findPathway2NearestConstruction( size/2 );
  }   

  if( !pathway.isValid() )
  {
    pathway = PathwayHelper::create( pos(), _city()->getBorderInfo( PlayerCity::roadExit ).epos(),
                                     PathwayHelper::allTerrain );
    setTarget( TilePos::invalid() );
  }

  if( !pathway.isValid() )
  {
    pathway = PathwayHelper::randomWay( _city(), pos(), 10 );
    setTarget( TilePos::invalid() );
    _failedWayCounter++;
  }

  if( _failedWayCounter > 4 )
  {
    pathway = Pathway();
  }

  if( pathway.isValid() )
  {
    _setSubAction( target().i() >= 0 ? go2enemy : go2position );
    _updatePathway( pathway );
    go();
  }
  else
  {
    //impossible state, but...
    Logger::warning( "EnemySoldier: can't find any path" );
    die();
  }
}

ConstructionList EnemySoldier::_findContructionsInRange( unsigned int range )
{
  ConstructionList ret;
  Tilemap& tmap = _map();

  for( unsigned int k=0; k <= range; k++ )
  {
    ConstructionList blds = tmap.rect( k, pos() )
                                .overlays()
                                .select<Construction>();

    for( auto b : blds )
    {
      if( !_atExclude.count( b->group() ) )
      {
        ret.push_back( b );
      }
    }
  }

  if( ret.empty() )
    return ret;

  switch( _atPriority )
  {
  case attackFood:
  case attackCitizen:
  case attackIndustry:
  {
    ConstructionList tmpRet;
    object::Group needGroup;
    switch( _atPriority )
    {
    case attackIndustry: needGroup = object::group::industry; break;
    case attackFood: needGroup = object::group::food; break;
    case attackCitizen:  needGroup = object::group::house; break;
    case attackSenate: needGroup = object::group::administration; break;
    default: needGroup = object::group::unknown; break;
    }

    for( auto bld : ret )
    {
      if( bld->group() == needGroup )
      {
        tmpRet << bld;
      }
    }

    if( !tmpRet.empty() )
      return tmpRet;
  }
  break;

  case attackBestBuilding:
  {
    ConstructionPtr maxBuilding = ret.front();
    unsigned int maxCost = maxBuilding->info().cost();

    for( auto& it : ret )
    {
      unsigned int cost = it->info().cost();
      if( cost > maxCost )
      {
        maxCost = cost;
        maxBuilding = it;
      }
    }

    if( maxBuilding.isValid() )
    {
      ret.clear();
      ret.push_back( maxBuilding );
      return ret;
    }
  }
  break;

  default:
  break;
  }

  return ret;
}

Pathway EnemySoldier::_findPathway2NearestConstruction( unsigned int range )
{
  Pathway ret;

  ConstructionList constructions = _findContructionsInRange( range );

  for( auto c : constructions )
  {
    ret = PathwayHelper::create( pos(), c, PathwayHelper::allTerrain );
    if( ret.isValid() )
    {
      setTarget( c->pos() );
      return ret;
    }
  }

  return Pathway();
}

void EnemySoldier::_centerTile()
{
  switch( _subAction() )
  {
  case doNothing:
  break;

  case check4attack: _check4attack(); break;

  case go2position:
  {
    if( _tryAttack() )
      return;

    if( target().i() < 0 )
    {
      _check4attack();
    }
  }
  break;

  default:
  break;
  }
  Walker::_centerTile();
}

void EnemySoldier::timeStep(const unsigned long time)
{
  Soldier::timeStep( time );

  switch( _subAction() )
  {
  case fightEnemy:
  {
    WalkerList enemies = _findEnemiesInRange( attackDistance() );

    if( !enemies.empty() )
    {
      WalkerPtr p = enemies.front();
      turn( p->pos() );
      p->updateHealth( -3 );
      p->acceptAction( Walker::acFight, pos() );
    }
    else
    {
      _check4attack();
    }
  }
  break;

  case destroyBuilding:
  {    
    ConstructionList constructions;
    ConstructionPtr c = _map().overlay<Construction>( target() );

    if( c.isValid() && !_atExclude.count( c->group() ) )
    {
      constructions << c;
    }

    if( constructions.empty() )
    {
      constructions = _findContructionsInRange( attackDistance() );
    }

    if( !constructions.empty() )
    {
      ConstructionPtr b = constructions.front();

      turn( b->pos() );
      b->updateState( pr::damage, 1 );
    }
    else
    {
      _check4attack();
    }
  }

  default: break;
  } // end switch( _d->action )
}

EnemySoldier::~EnemySoldier() {}

int EnemySoldier::agressive() const { return 2; }

void EnemySoldier::send2City( TilePos pos )
{
  setPos( pos );
  _check4attack();
  attach();

  events::dispatch<events::MilitaryThreat>( 1 );
}

bool EnemySoldier::die()
{
  bool created = Soldier::die();

  if( !created )
  {
    Corpse::create( _city(), pos(), ResourceGroup::celts, 393, 400 );
    return true;
  }

  return created;
}

void EnemySoldier::setAttackPriority(EnemySoldier::AttackPriority who) {_atPriority = who;}

void EnemySoldier::acceptAction(Walker::Action action, TilePos pos)
{
  Soldier::acceptAction( action, pos );
  if( _subAction() != EnemySoldier::fightEnemy && action == Walker::acFight )
  {
    _tryAttack();
  }
}

void EnemySoldier::load( const VariantMap& stream )
{
  Soldier::load( stream );

  VARIANT_LOAD_ANY( _failedWayCounter, stream )
}

void EnemySoldier::save( VariantMap& stream ) const
{
  Soldier::save( stream );

  VARIANT_SAVE_ANY( stream, _failedWayCounter )
  stream[ "type" ] = (int)type();
  stream[ "__debug_typeName" ] = Variant( WalkerHelper::getTypename( type() ) );
}
