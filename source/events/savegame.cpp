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

#include "savegame.hpp"
#include "game/game.hpp"
#include "world/empire.hpp"
#include "world/emperor.hpp"
#include "gui/dialogbox.hpp"
#include "game/settings.hpp"
#include "gui/environment.hpp"
#include "core/gettext.hpp"
#include "core/logger.hpp"
#include "gui/save_dialog.hpp"

using namespace gui;
using namespace dialog;

namespace events
{

GameEventPtr ShowSaveDialog::create()
{
  GameEventPtr ret( new ShowSaveDialog() );
  ret->drop();

  return ret;
}

void ShowSaveDialog::_exec(Game& game, unsigned int)
{
  vfs::Directory saveDir = SETTINGS_STR( savedir );
  std::string defaultExt = SETTINGS_STR( saveExt );

  if( !saveDir.exist() )
  {
    Dialog* dialog = Information( game.gui(),
                                  _("##warning##"),
                                  _("##save_directory_not_exist##") );
    dialog->show();
    return;
  }

  auto& dialog = game.gui()->add<SaveGame>( saveDir, defaultExt, -1 );
  CONNECT( &dialog, onFileSelected(), &game, Game::save )
}

bool ShowSaveDialog::_mayExec(Game&, unsigned int) const{ return true; }

ShowSaveDialog::ShowSaveDialog() {}

}
