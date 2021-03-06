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
// Copyright 2012-2015 Dalerank, dalerankn8@gmail.com

#include "texturedbutton.hpp"
#include "game/resourcegroup.hpp"
#include "environment.hpp"
#include "widget_factory.hpp"
#include "dictionary.hpp"

namespace gui
{

REGISTER_CLASS_IN_WIDGETFACTORY(TexturedButton)
REGISTER_CLASS_IN_WIDGETFACTORY(HelpButton)
REGISTER_CLASS_IN_WIDGETFACTORY(ExitButton)

TexturedButton::TexturedButton(Widget* parent, const Point& pos, const TexturedButton::States& states)
  : TexturedButton( parent, pos, Size( defaultSize ), -1, states )
{

}

TexturedButton::TexturedButton(Widget *parent, const Point &pos, const Size &size, int id, const States& states)
  : PushButton( parent, Rect( pos, size ), "", id, false, noBackground )
{
  setPicture( gui::rc.panel, states.normal, stNormal );
  setPicture( gui::rc.panel, (states.hover == -1) ? states.normal+1 : states.hover, stHovered );
  setPicture( gui::rc.panel, (states.pressed == -1) ? states.normal+2 : states.pressed, stPressed );
  setPicture( gui::rc.panel, (states.disabled == -1) ? states.normal+3 : states.disabled, stDisabled );
  setTextVisible( false );
}

TexturedButton::TexturedButton(Widget *parent, const Point &pos, const Size &size, int id,
                               const std::string& resourceGroup, const States& states)
  : PushButton( parent, Rect( pos, size ), "", id, false, noBackground )
{
  setPicture( resourceGroup, states.normal, stNormal );
  setPicture( resourceGroup, (states.hover == -1) ? states.normal+1 : states.hover, stHovered  );
  setPicture( resourceGroup, (states.pressed == -1) ? states.normal+2 : states.pressed, stPressed  );
  setPicture( resourceGroup, (states.disabled == -1) ? states.normal+3 : states.disabled, stDisabled );
  setTextVisible( false );
}

gui::TexturedButton::TexturedButton(gui::Widget *parent) : PushButton( parent )
{
  setTextVisible( false );
}

HelpButton::HelpButton(Widget* parent)
  : TexturedButton( parent )
{

}

HelpButton::HelpButton(Widget* parent, const Point& pos, const std::string& helpId, int id)
  : TexturedButton( parent, pos, Size( defaultSize ), id, States( gui::button.help ) )
{
  _helpid = helpId;
}

void HelpButton::setupUI(const VariantMap& ui)
{
  TexturedButton::setupUI( ui );
  _helpid = ui.get( "uri" ).toString();
}

void HelpButton::_btnClicked()
{
  if( !_helpid.empty() )
    ui()->add<DictionaryWindow>( _helpid  );
}

ExitButton::ExitButton(Widget* parent)
  : TexturedButton( parent )
{

}

ExitButton::ExitButton(Widget* parent, const Point& pos, int id)
  : TexturedButton( parent, pos, Size( defaultSize ), id, States( gui::button.exit ) )
{

}

void ExitButton::_btnClicked()
{
  if( parent() )
    parent()->deleteLater();
}

}//end namespace gui
