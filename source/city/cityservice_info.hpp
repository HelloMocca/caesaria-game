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

#ifndef __CAESARIA_CITYSERVICE_INFO_H_INCLUDED__
#define __CAESARIA_CITYSERVICE_INFO_H_INCLUDED__

#include "cityservice.hpp"
#include "game/predefinitions.hpp"

namespace city
{

PREDEFINE_CLASS_SMARTPOINTER(Info)

class Info : public Srvc
{
public:
  enum { lastMonth=1 };
  typedef enum { population=0, funds, tax, taxpayes,
                 monthWithFood, foodKoeff, godsMood,
                 needWorkers, maxWorkers, workless,
                 crimeLevel, colloseumCoverage, theaterCoverage,
                 entertainment, lifeValue, education,
                 payDiff, monthWtWar, cityWages,
                 romeWages, peace, milthreat,
                 houseNumber, slumNumber, shackNumber,
                 sentiment, foodStock, foodMontlyConsumption,
                 favour, prosperity, blackHouses,
                 paramsCount } ParamName;

  class Parameters : public std::vector<int>
  {
  public:    
    DateTime date;

    Parameters();
    Parameters( const Parameters& other );

    VariantList save() const;
    void load(const VariantList& stream );
  };

  struct MaxParameterValue
  {
    DateTime date;
    int value;
  };  

  class History : public std::vector<Parameters>
  {
  public:
    VariantMap save() const;
    void load( const VariantMap& vm );
  };

  class MaxParameters : public std::vector< MaxParameterValue >
  {
  public:
    VariantMap save() const;
    void load( const VariantMap& vm );
  };

  virtual void timeStep( const unsigned int time );
  Parameters lastParams() const;
  Parameters params( unsigned int monthAgo ) const;
  Parameters yearParams( unsigned int year ) const;
  const MaxParameters& maxParams() const;

  const History& history() const;

  static std::string defaultName();

  virtual VariantMap save() const;
  virtual void load(const VariantMap& stream);

  Info(PlayerCityPtr city);
private:

  class Impl;
  ScopedPtr< Impl > _d;
};

}//end namespace city

#endif //__CAESARIA_CITYSERVICE_INFO_H_INCLUDED__
