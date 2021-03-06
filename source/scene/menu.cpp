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
// Copyright 2012-2015 Dalerank, dalerankn8@gmail.com

#include "menu.hpp"

#include "core/gettext.hpp"
#include "gui/loadgamedialog.hpp"
#include "gfx/engine.hpp"
#include "core/exception.hpp"
#include "gui/startmenu.hpp"
#include "gui/environment.hpp"
#include "game/game.hpp"
#include "game/player.hpp"
#include "gui/pushbutton.hpp"
#include "gui/label.hpp"
#include "game/settings.hpp"
#include "core/color_list.hpp"
#include "gui/playername_window.hpp"
#include "core/logger.hpp"
#include "core/foreach.hpp"
#include "vfs/directory.hpp"
#include "gui/label.hpp"
#include "gui/listbox.hpp"
#include "core/locale.hpp"
#include "core/saveadapter.hpp"
#include "gui/smkviewer.hpp"
#include "gui/dialogbox.hpp"
#include "core/osystem.hpp"
#include "gui/texturedbutton.hpp"
#include "sound/engine.hpp"
#include "events/setvideooptions.hpp"
#include "events/setsoundoptions.hpp"
#include "gui/widgetpositionanimator.hpp"
#include "gui/loadmissiondialog.hpp"
#include "gui/widgetescapecloser.hpp"
#include "core/event.hpp"
#include "gui/package_options_window.hpp"
#include "core/timer.hpp"
#include "core/variant_map.hpp"
#include "events/dispatcher.hpp"
#include "core/utils.hpp"
#include "walker/name_generator.hpp"
#include "gui/image.hpp"
#include "vfs/directory.hpp"
#include "gui/dlc_folder_viewer.hpp"
#include "steam.hpp"
#include "gui/window_language_select.hpp"

using namespace gfx;
using namespace gui;

namespace scene
{

class StartMenu::Impl
{
public:
  Picture bgPicture;
  Point bgOffset;
  gui::StartMenu* menu;         // menu to display
  bool isStopped;
  Game* game;
  Engine* engine;
  std::string fileMap;
  int result;

  Picture userImage;
  gui::Label* lbSteamName;

public:
  void handleNewGame();
  void showCredits();
  void showLoadMenu();
  void showNewGame();
  void showOptionsMenu();
  void playRandomap();
  void constructorMode();
  void showMainMenu();
  void showSoundOptions();
  void showVideoOptions();
  void showMissionSelector();
  void quitGame();
  void selectFile( std::string fileName );
  void setPlayerName( std::string name );
  void openSteamPage();
  void openHomePage();
  void showMapSelectDialog();
  void showSaveSelectDialog();
  void changePlayerName();
  void showAdvancedMaterials();
  void startCareer();
  void showLanguageOptions();
  void showPackageOptions();
  void changeLanguage(std::string lang, std::string newFont, std::string sounds);
  void fitScreenResolution();
  void playMenuSoundTheme();
  void continuePlay();
  void resolveSteamStats();
  void changePlayerNameIfNeed(bool force=false);
  void reload();
  void restart();
  void openDlcDirectory(Widget* sender);
  void showLogFile();
  gui::Ui& ui();
};

void StartMenu::Impl::showSaveSelectDialog()
{
  vfs::Path savesPath = SETTINGS_STR( savedir );

  result = StartMenu::loadSavedGame;
  auto& loadGameDialog = ui().add<dialog::LoadGame>( savesPath );
  loadGameDialog.setShowExtension( false );
  loadGameDialog.setMayDelete( true );

  CONNECT( &loadGameDialog, onSelectFile(), this, Impl::selectFile );
  loadGameDialog.setTitle( _("##mainmenu_loadgame##") );
  loadGameDialog.setText( _("##load_this_game##") );

  changePlayerNameIfNeed();
}

void StartMenu::Impl::changePlayerName() { changePlayerNameIfNeed(true); }

void StartMenu::Impl::showLogFile()
{
  vfs::Directory logfile = SETTINGS_STR( workDir );
  logfile = logfile/SETTINGS_STR( logfile );
  OSystem::openUrl( logfile.toString(), steamapi::ld_prefix() );
}

void StartMenu::Impl::changePlayerNameIfNeed(bool force)
{
  std::string playerName = SETTINGS_STR( playerName );
  if( playerName.empty() || force )
  {
    auto& dlg = ui().add<dialog::ChangePlayerName>();
    dlg.setName( playerName );
    dlg.setMayExit( false );

    CONNECT( &dlg, onNameChange(), this, Impl::setPlayerName );
    CONNECT( &dlg, onContinue(), &dlg, dialog::ChangePlayerName::deleteLater );
  }
}

void StartMenu::Impl::fitScreenResolution()
{
  gfx::Engine::Modes modes = game->engine()->modes();
  SETTINGS_SET_VALUE( resolution, modes.front() );
  SETTINGS_SET_VALUE( fullscreen, true );
  SETTINGS_SET_VALUE( screenFitted, true );
  game::Settings::save();

  dialog::Information( &ui(), "", "Enabled fullscreen mode. Please restart game");
  //CONNECT( dlg, onOk(), this, Impl::restart );
}

void StartMenu::Impl::playMenuSoundTheme()
{
  audio::Engine::instance().play( "main_menu", 50, audio::theme );
}

void StartMenu::Impl::continuePlay()
{
  result = StartMenu::loadSavedGame;
  selectFile( SETTINGS_STR( lastGame ) );
}

void StartMenu::Impl::resolveSteamStats()
{
  if( steamapi::available() )
  {
    int offset = 0;
    for( int k=0; k < steamapi::achv_count; k++ )
    {
      auto achieventId = steamapi::AchievementType(k);
      if( steamapi::isAchievementReached( achieventId ) )
      {
        gfx::Picture pic = steamapi::achievementImage( achieventId );
        if( pic.isValid() )
        {
          auto& img = ui().add<gui::Image>( Point( 10, 100 + offset ), pic );
          img.setTooltipText( steamapi::achievementCaption( achieventId ) );
          offset += 65;
        }
      }
    }
  }
}

void StartMenu::Impl::reload()
{
  result = StartMenu::reloadScreen;
  isStopped = true;
}

void StartMenu::Impl::restart()
{
  std::string filename;

  if( OSystem::isLinux() ) filename = "caesaria.linux";
  else if( OSystem::isWindows() ) filename = "caesaria.exe";
  else if( OSystem::isMac() ) filename = "caesaria.macos";
  else filename = "unknown";

  vfs::Directory appDir = vfs::Directory::applicationDir();
  vfs::Path appFile = appDir/vfs::Path(filename);
  OSystem::restartProcess( appFile.toString(), appDir.toString(), StringArray() );
}

void StartMenu::Impl::openDlcDirectory(Widget* sender)
{
  if( sender == 0 )
    return;

  vfs::Path path( sender->getProperty( "path" ).toString() );
  ui().add<DlcFolderViewer>( path );
}

void StartMenu::Impl::showSoundOptions()
{
  events::dispatch<events::ChangeSoundOptions>();
}

void StartMenu::Impl::showLanguageOptions()
{
  auto& languageSelectDlg = ui().add<dialog::LanguageSelect>( SETTINGS_RC_PATH( langModel ),
                                                              SETTINGS_STR( language ) );
  languageSelectDlg.setDefaultFont( SETTINGS_STR( defaultFont ) );

  CONNECT_LOCAL( &languageSelectDlg, onChange,   Impl::changeLanguage )
  CONNECT_LOCAL( &languageSelectDlg, onContinue, Impl::reload         )
}

void StartMenu::Impl::showPackageOptions()
{
  ui().add<dialog::PackageOptions>( Rect() );
}

void StartMenu::Impl::changeLanguage(std::string lang, std::string newFont, std::string sounds)
{  
  std::string currentFont = SETTINGS_STR( font );

  SETTINGS_SET_VALUE( language, Variant( lang ) );
  SETTINGS_SET_VALUE( talksArchive, Variant( sounds ) );

  if( currentFont != newFont )
  {
    SETTINGS_SET_VALUE( font, newFont );
    FontCollection::instance().initialize( game::Settings::rcpath().toString(), newFont );
  }

  game::Settings::save();

  Locale::setLanguage( lang );
  NameGenerator::instance().setLanguage( lang );
  audio::Helper::initTalksArchive( sounds );
}

void StartMenu::Impl::startCareer()
{
  menu->clear();

  std::string playerName = SETTINGS_STR( playerName );

  auto& selectPlayerNameDlg = ui().add<dialog::ChangePlayerName>();
  selectPlayerNameDlg.setName( playerName );

  CONNECT_LOCAL( &selectPlayerNameDlg, onNameChange(), Impl::setPlayerName );
  CONNECT_LOCAL( &selectPlayerNameDlg, onContinue(),   Impl::handleNewGame );
  CONNECT_LOCAL( &selectPlayerNameDlg, onClose(),      Impl::showMainMenu  );
}

void StartMenu::Impl::handleNewGame()
{  
  result=startNewGame; isStopped=true;
}

void StartMenu::Impl::showCredits()
{
  audio::Engine::instance().play( "combat_long", 50, audio::theme );

  std::string strs[] = { _("##original_game##"),
                         "Caesar III (c)",
                         "Thank you, Impressions Games, for amazing game",
                         " ",
                         _("##developers##"),
                         " ",
                         "dalerank (dalerankn8@gmail.com)",
                         "gathanase (gathanase@gmail.com) render, game mechanics ",
                         "gecube (gb12335@gmail.com), Softer (softer@lin.in.ua)",
                         "pecunia (pecunia@heavengames.com) game mechanics",
                         "amdmi3 (amdmi3@amdmi3.ru) bsd fixes",
                         "greg kennedy(kennedy.greg@gmail.com) smk decoder",
                         "akuskis (?) aqueduct system",
                         "VladRassokhin, hellium, andreibranescu",
                         " ",
                         _("##operations_manager##"),
                         " ",
                         "Max Mironchik (?) ",
                         " ",
                         _("##testers##"),
                         " ",
                         "radek liška, dimitrius (caesar-iii.ru)",
                         "shibanirm, Pavel Aleksandrov (krabanek@gmail.com)",
                         " ",
                         _("##graphics##"),
                         " ",
                         "Dmitry Plotnikov (main artist)",
                         "Jennifer Kin (empire map, icons)",
                         "Andre Lisket (school, theater, baths and others)",
                         "Il'ya Korchagin (grape farm tiles)",
                         "Pietro Chiovaro (Hospital)",
                         " ",
                         _("##music##"),
                         " ",
                         "Aliaksandr BeatCheat (sounds)",
                         "Omri Lahav (main theme)",
                         "Kevin MacLeod (ambient, game themes)",
                         " ",
                         _("##localization##"),
                         " ",
                         "Alexander Klimenko, Manuel Alvarez, Artem Tolmachev, Peter Willington, Leszek Bochenek",
                         "Michele Ribechini",
                         " ",
                         _("##thanks_to##"),
                         " ",
                         "vk.com/caesaria-game, dimitrius (caesar-iii.ru), aneurysm (4pda.ru)",
                         "Aleksandr Egorov, Juan Font Alonso, Mephistopheles",
                         "ed19837, vladimir.rurukin, Safronov Alexey, Alexander Skidanov",
                         "Kostyantyn Moroz, Andrew, Nikita Gradovich, bogdhnu",
                         "deniskravtsov, Vhall, Dmitry Vorobiev, yevg.mord",
                         "mmagir,Yury Vidineev, Pavel Aleynikov, brickbtv",
                         "dovg1, Konstantin Kitmanov, Serge Savostin, Memfis",
                         "MennyCalavera, Anastasia Smolskaya, niosus, SkidanovAlex",
                         "Zatolokinandrey, yuri_abzyanov, dmitrii.dukhonchenko, twilight.temple",
                         "holubmarek,butjer1010, Agmenor Ultime, m0nochr0mex, Alexeyco",
                         "rad.n,j simek.cz, saintech, phdarcy, Casey Knauss, meikit2000",
                         "victor sosa, ImperatorPrime, nickers, veprbl, ramMASTER",
                         "tracertong, pufik6666, rovanion",
                         "" };

  Size size = ui().vsize();
  Label& frame = ui().add<Label>( Rect( Point( 0, 0), size ), "", false, gui::Label::bgSimpleBlack );
  WidgetEscapeCloser::insertTo( &frame );
  frame.setAlpha( 0xa0 );
  int h = size.height();
  for( int i=0; !strs[i].empty(); i++ )
  {
    Label& lb = frame.add<Label>( Rect( 0, h + i * 20, size.width(), h + (i + 1) * 20), strs[i] );
    lb.setTextAlignment( align::center, align::center );
    lb.setFont( Font::create( FONT_2_WHITE ) );
    lb.setSubElement( true );
    auto& animator = lb.add<PositionAnimator>( WidgetAnimator::removeSelf | WidgetAnimator::removeParent, Point( 0, -20), 10000 );
    animator.setSpeed( PointF( 0, -0.5 ) );
  }

  auto& buttonClose = frame.add<PushButton>( Rect( size.width() - 150, size.height() - 34, size.width() - 10, size.height() - 10 ),
                                             _("##close##") );
  frame.setFocus();

  CONNECT( &buttonClose, onClicked(), &frame, Label::deleteLater );
  CONNECT( &buttonClose, onClicked(), this, Impl::playMenuSoundTheme );
}

#define ADD_MENU_BUTTON( text, slot) { auto& btn = menu->addButton( _(text), -1 ); CONNECT( &btn, onClicked(), this, slot ); }

void StartMenu::Impl::showLoadMenu()
{
  menu->clear();

  ADD_MENU_BUTTON( "##mainmenu_playmission##", Impl::showMissionSelector )
  ADD_MENU_BUTTON( "##mainmenu_loadgame##",    Impl::showSaveSelectDialog )
  ADD_MENU_BUTTON( "##mainmenu_loadmap##",     Impl::showMapSelectDialog )
  ADD_MENU_BUTTON( "##cancel##",               Impl::showMainMenu )
}

void StartMenu::Impl::constructorMode()
{
  auto& loadFileDialog = ui().add<dialog::LoadFile>( Rect(),
                                                     vfs::Path( ":/maps/" ), ".map,.sav,.omap",
                                                     -1 );
  loadFileDialog.setMayDelete( false );

  result = StartMenu::loadConstructor;
  CONNECT( &loadFileDialog, onSelectFile(), this, Impl::selectFile );
  loadFileDialog.setTitle( _("##mainmenu_loadmap##") );
  loadFileDialog.setText( _("##start_this_map##") );

  changePlayerNameIfNeed();

}

void StartMenu::Impl::playRandomap()
{
  result = StartMenu::loadMission;
  fileMap = ":/missions/random.mission";
  isStopped = true;
}

void StartMenu::Impl::showOptionsMenu()
{
  menu->clear();

  ADD_MENU_BUTTON( "##mainmenu_language##", Impl::showLanguageOptions )
  ADD_MENU_BUTTON( "##mainmenu_video##",    Impl::showVideoOptions )
  ADD_MENU_BUTTON( "##mainmenu_sound##",    Impl::showSoundOptions )
  ADD_MENU_BUTTON( "##mainmenu_package##",  Impl::showPackageOptions )
  ADD_MENU_BUTTON( "##mainmenu_plname##",   Impl::changePlayerName )
  ADD_MENU_BUTTON( "##mainmenu_showlog##",  Impl::showLogFile )
  ADD_MENU_BUTTON( "##cancel##",            Impl::showMainMenu )
}

void StartMenu::Impl::showNewGame()
{
  menu->clear();

  ADD_MENU_BUTTON( "##mainmenu_startcareer##", Impl::startCareer )
  ADD_MENU_BUTTON( "##mainmenu_randommap##",   Impl::playRandomap )
  ADD_MENU_BUTTON( "##mainmenu_constructor##", Impl::constructorMode )
  ADD_MENU_BUTTON( "##cancel##",               Impl::showMainMenu )
}

void StartMenu::Impl::showMainMenu()
{
  menu->clear();

  std::string lastGame = SETTINGS_STR( lastGame );
  if( !lastGame.empty() )
    ADD_MENU_BUTTON( "##mainmenu_continueplay##", Impl::continuePlay )

  ADD_MENU_BUTTON( "##mainmenu_newgame##",        Impl::showNewGame )
  ADD_MENU_BUTTON( "##mainmenu_load##",           Impl::showLoadMenu )
  ADD_MENU_BUTTON( "##mainmenu_options##",        Impl::showOptionsMenu )
  ADD_MENU_BUTTON( "##mainmenu_credits##",        Impl::showCredits )

  if( vfs::Path( ":/dlc" ).exist() )
  {
    ADD_MENU_BUTTON( "##mainmenu_mcmxcviii##",    Impl::showAdvancedMaterials )
  }

  ADD_MENU_BUTTON( "##mainmenu_quit##",           Impl::quitGame )
}

void StartMenu::Impl::showAdvancedMaterials()
{
  menu->clear();

  vfs::Directory dir( std::string( ":/dlc" ) );
  if( !dir.exist() )
  {
    auto infoDialog = dialog::Information( menu->ui(), _("##no_dlc_found_title##"), _("##no_dlc_found_text##"));
    infoDialog->show();
    showMainMenu();
    return;
  }

  StringArray excludeFolders;
  excludeFolders << vfs::Path::firstEntry << vfs::Path::secondEntry;
  vfs::Entries::Items entries = dir.entries().items();
  for( auto& it : entries )
  {
    if( it.isDirectory )
    {
      if( excludeFolders.contains( it.name.toString() ) )
        continue;

      vfs::Path path2subdir = it.fullpath;
      std::string locText = "##mainmenu_dlc_" + path2subdir.baseName().toString() + "##";

      auto& btn = menu->addButton( _(locText), -1 );
      btn.addProperty( "path", Variant( path2subdir.toString() ) );
      CONNECT( &btn, onClickedEx(), this, Impl::openDlcDirectory )
    }
  }

  ADD_MENU_BUTTON( "##cancel##", Impl::showMainMenu )
}

void StartMenu::Impl::showVideoOptions()
{
  events::dispatch<events::SetVideoSettings>();
}

void StartMenu::Impl::showMissionSelector()
{
  result = StartMenu::loadMission;
  auto& wnd = ui().add<dialog::LoadMission>( vfs::Path( ":/missions/" ) );

  CONNECT( &wnd, onSelectFile(), this, Impl::selectFile );

  changePlayerNameIfNeed();
}

void StartMenu::Impl::quitGame()
{
  game::Settings::save();
  result=closeApplication;
  isStopped=true;
}

void StartMenu::Impl::selectFile(std::string fileName)
{
  fileMap = fileName;
  isStopped = true;
}

void StartMenu::Impl::setPlayerName(std::string name) { SETTINGS_SET_VALUE( playerName, Variant( name ) ); }
void StartMenu::Impl::openSteamPage() { OSystem::openUrl( "http://store.steampowered.com/app/327640", steamapi::ld_prefix() ); }
void StartMenu::Impl::openHomePage() { OSystem::openUrl( "http://www.caesaria.net", steamapi::ld_prefix() ); }

void StartMenu::Impl::showMapSelectDialog()
{
  auto&& loadFileDialog = ui().add<dialog::LoadFile>( Rect(),
                                                      vfs::Path( ":/maps/" ), ".map,.sav,.omap",
                                                      -1 );
  loadFileDialog.setMayDelete( false );

  result = StartMenu::loadMap;
  CONNECT( &loadFileDialog, onSelectFile(), this, Impl::selectFile );
  loadFileDialog.setTitle( _("##mainmenu_loadmap##") );
  loadFileDialog.setText( _("##start_this_map##") );

  changePlayerNameIfNeed();
}

StartMenu::StartMenu( Game& game, Engine& engine ) : _d( new Impl )
{
  _d->bgPicture = Picture::getInvalid();
  _d->isStopped = false;
  _d->game = &game;
  _d->userImage = Picture::getInvalid();
  _d->engine = &engine;
}

StartMenu::~StartMenu() {}

void StartMenu::draw()
{
  _d->ui().beforeDraw();
  _d->engine->draw(_d->bgPicture, _d->bgOffset);
  _d->ui().draw();

  if( steamapi::available() )
  {
    _d->engine->draw( _d->userImage, Point( 20, 20 ) );
  }
}

void StartMenu::handleEvent( NEvent& event )
{
  if (event.EventType == sEventQuit)
  {
    _d->quitGame();
  }

  _d->ui().handleEvent( event );
}

void StartMenu::initialize()
{
  events::Dispatcher::instance().reset();
  Logger::warning( "ScreenMenu: initialize start");
  std::string resName = SETTINGS_STR( titleResource );
  _d->bgPicture.load( resName, 1);

  // center the bgPicture on the screen
  Size tmpSize = (_d->ui().vsize() - _d->bgPicture.size())/2;
  _d->bgOffset = Point( tmpSize.width(), tmpSize.height() );

  _d->ui().clear();

  _d->menu = &_d->ui().add<gui::StartMenu>();

  Size scrSize = _d->ui().vsize();
  auto& btnHomePage = _d->ui().add<TexturedButton>( Point( scrSize.width() - 128, scrSize.height() - 100 ), Size( 128 ), -1,
                                                    "logo_rdt", TexturedButton::States( 1, 2, 2, 2 ) );

  auto& btnSteamPage = _d->ui().add<TexturedButton>( Point( btnHomePage.left() - 128, scrSize.height() - 100 ),  Size( 128 ), -1,
                                                     "steam_icon", TexturedButton::States( 1, 2, 2, 2 ) );

  CONNECT( &btnSteamPage, onClicked(), _d.data(), Impl::openSteamPage );
  CONNECT( &btnHomePage, onClicked(), _d.data(), Impl::openHomePage );

  _d->showMainMenu();

  if( OSystem::isAndroid() )
  {
    bool screenFitted = KILLSWITCH( screenFitted ) || KILLSWITCH( fullscreen );
    if( !screenFitted )
    {
      Rect dialogRect = Rect( 0, 0, 400, 150 );
      auto& dialog = _d->ui().add<dialog::Dialog>( dialogRect,
                                                    "Information", "Is need autofit screen resolution?",
                                                    dialog::Dialog::btnOkCancel );
      CONNECT( &dialog, onOk(),     &dialog, dialog::Dialog::deleteLater );
      CONNECT( &dialog, onCancel(), &dialog, dialog::Dialog::deleteLater );
      CONNECT( &dialog, onOk(),     _d.data(), Impl::fitScreenResolution );
      SETTINGS_SET_VALUE(screenFitted, true);

      dialog.show();
    }
  }

  if( !OSystem::isAndroid() )
    _d->playMenuSoundTheme();

  if( steamapi::available() )
  {
    steamapi::init();

    std::string steamName = steamapi::userName();

    std::string lastName = SETTINGS_STR( playerName );
    if( lastName.empty() )
      SETTINGS_SET_VALUE( playerName, Variant( steamName ) );

    _d->userImage = steamapi::userImage();
    if( steamName.empty() )
    {
      OSystem::error( "Error", "Can't login in Steam" );
      _d->isStopped = true;
      _d->result = closeApplication;
      return;
    }

    std::string text = fmt::format( "Build {0}\n{1}", GAME_BUILD_NUMBER, steamName );
    _d->lbSteamName = &_d->ui().add<Label>( Rect( 100, 10, 400, 80 ), text );
    _d->lbSteamName->setTextAlignment( align::upperLeft, align::center );
    _d->lbSteamName->setWordwrap( true );
    _d->lbSteamName->setFont( Font::create( FONT_3, ColorList::white ) );
  }
}

void StartMenu::afterFrame()
{
  Base::afterFrame();

  static unsigned int saveTime = 0;
  events::Dispatcher::instance().update( *_d->game, saveTime++ );

  if( steamapi::available() )
  {
    steamapi::update();
    if( steamapi::isStatsReceived() )
      _d->resolveSteamStats();
  }
}

int StartMenu::result() const{  return _d->result;}
bool StartMenu::isStopped() const{  return _d->isStopped;}
Ui& StartMenu::Impl::ui() { return *game->gui(); }
std::string StartMenu::mapName() const{  return _d->fileMap;}
std::string StartMenu::playerName() const { return SETTINGS_STR( playerName ); }

}//end namespace scene
