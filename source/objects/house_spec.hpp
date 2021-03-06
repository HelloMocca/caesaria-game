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
// Copyright 2012-2013 Gregoire Athanase, gathanase@gmail.com
// Copyright 2012-2014 Dalerank, dalerankn8@gmail.com

#ifndef _CAESARIA_HOUSE_SPEC_H_INCLUDE_
#define _CAESARIA_HOUSE_SPEC_H_INCLUDE_

#include "good/good.hpp"
#include "core/scopedptr.hpp"
#include "core/smartptr.hpp"
#include "vfs/path.hpp"
#include "predefinitions.hpp"
#include "house_level.hpp"
#include "service.hpp"

class HouseSpecification
{
  friend class HouseSpecHelper;

public:
  typedef enum { intv_foods=0, intv_goods, intv_service, intv_count } IntervalName;
  enum { needTheater=1, needAmphitheater=2, needColosseum=3 };

  HouseLevel::ID level() const;
  int tileCapacity() const;
  int taxRate() const;
  int prosperity() const;
  int crime() const;

  // return the house type "small casa, luxury villa, ..."
  const std::string& levelName() const;
  const std::string& internalName() const;

  int getRequiredGoodLevel( good::Product type) const;

  // returns True if patrician villa
  bool isPatrician() const;

  bool checkHouse(HousePtr house, std::string* retMissing = 0,
                  object::Type* needBuilding = 0, TilePos *retPos=0) const;

  unsigned int consumptionInterval( IntervalName name ) const;

  int findLowLevelHouseNearby(HousePtr house, TilePos &refPos) const;
  int findUnwishedBuildingNearby(HousePtr house, object::Type& rType, TilePos &refPos) const;

  HouseSpecification next() const;

  int computeDesirabilityLevel(HousePtr house, std::string &oMissingRequirement) const;
  int computeEntertainmentLevel(HousePtr house) const;
  int computeEducationLevel(HousePtr house, std::string &oMissingRequirement) const;
  int computeHealthLevel(HousePtr house, std::string &oMissingRequirement) const;
  int computeReligionLevel(HousePtr house) const;
  int computeWaterLevel(HousePtr house, std::string &oMissingRequirement) const;
  int computeFoodLevel(HousePtr house) const;
  int computeMonthlyGoodConsumption(HousePtr house, const good::Product goodType, bool real) const;
  int computeMonthlyFoodConsumption( HousePtr house ) const;

  float evaluateServiceNeed(HousePtr house, const Service::Type service);
  float evaluateEntertainmentNeed(HousePtr house, const Service::Type service);
  float evaluateEducationNeed(HousePtr house, const Service::Type service);
  float evaluateHealthNeed(HousePtr house, const Service::Type service);
  float evaluateReligionNeed(HousePtr house, const Service::Type service);
  // float evaluateFoodNeed(House &house, const ServiceType service);

  int minDesirabilityLevel() const;
  int maxDesirabilityLevel() const;
  int minEntertainmentLevel() const;
  int minEducationLevel() const;
  int minHealthLevel() const;
  int minReligionLevel() const;
//    int getMinWaterLevel();
  int minFoodLevel() const;
  ~HouseSpecification();
  HouseSpecification();
  HouseSpecification( const HouseSpecification& other );
  HouseSpecification& operator=(const HouseSpecification& other );

private:
  class Impl;
  ScopedPtr< Impl > _d;
};

class HouseSpecHelper
{
public:
  static HouseSpecHelper& instance();

  HouseSpecification getSpec(const int houseLevel);
  int getLevel( const std::string& name );
  void initialize( const vfs::Path& filename );
  gfx::Picture getPicture(int houseLevel , int size) const;

  ~HouseSpecHelper();
private:
  HouseSpecHelper();
  
  class Impl;
  ScopedPtr< Impl > _d;
};


#endif //_CAESARIA_HOUSE_SPEC_H_INCLUDE_
