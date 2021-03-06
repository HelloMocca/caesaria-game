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

#ifndef __CAESARIA_BARRACKS_H_INCLUDED__
#define __CAESARIA_BARRACKS_H_INCLUDED__

#include "training.hpp"
#include "good/good.hpp"

class Barracks : public TrainingBuilding
{
public:
  Barracks();
  virtual void deliverTrainee();
  virtual void timeStep(const unsigned long time);

  bool isNeedWeapons() const;
  int goodQty( good::Product type ) const;
  virtual void storeGoods( good::Stock &stock, const int amount);
  virtual std::string workersProblemDesc() const;

  virtual void save(VariantMap &stream) const;
  virtual void load(const VariantMap &stream);

private:
  class Impl;
  ScopedPtr< Impl > _d;
};

#endif //__CAESARIA_BARRACKS_H_INCLUDED__
