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

#include "smkviewer.hpp"
#include <smacker.h>
#include "gfx/picture.hpp"
#include "gfx/engine.hpp"
#include "core/logger.hpp"
#include "vfs/directory.hpp"
#include "widget_factory.hpp"

#include <stdio.h>

using namespace std;
using namespace gfx;

namespace gui
{

REGISTER_CLASS_IN_WIDGETFACTORY(SmkViewer)

class SmkViewer::Impl
{
public:
  SmkViewer::Mode mode;
  Picture background;
  bool needUpdateTexture;
  unsigned int lastFrameTime;
  smk smkfile;

  unsigned long smkfileWidth, smkfileHeight;
  unsigned long frameCount, currentFrame;

  double usecsInFrame;

  /* arrays for audio track metadata */
  unsigned char   a_trackmask, a_channels[7], a_depth[7];
  unsigned long   a_rate[7];

  //unsigned char *palette_data;
  unsigned char* image_data;
  unsigned char* pallete;

  int colors[256];

  void updateTexture(gfx::Engine& painter, const Size& size );
  void updatePallete();
  void nextFrame();

  Impl() : smkfile( 0 ), image_data( 0 ) {}

public signals:
  Signal0<> onFinishSignal;
};

//! constructor
SmkViewer::SmkViewer( Widget* parent )
  : Widget( parent, -1, Rect( 0, 0, 50, 50) ), _d( new Impl )
{
  _d->mode = native;
}

void SmkViewer::beforeDraw( gfx::Engine& painter )
{
  if( isFocused() && _d->smkfile != NULL && DateTime::elapsedTime() - _d->lastFrameTime > (_d->usecsInFrame / 1000) )
  {
    _d->lastFrameTime = DateTime::elapsedTime();
    _d->needUpdateTexture = true;

    _d->nextFrame();

    _d->updatePallete();
    /* Retrieve the palette and image */
    _d->image_data = smk_get_video( _d->smkfile );
  }

  if( _d->needUpdateTexture )
  {
    _d->needUpdateTexture = false;
    _d->updateTexture( painter, size() );
  }

  Widget::beforeDraw( painter );
}

void SmkViewer::setFilename(const vfs::Path& path)
{
  vfs::Directory dir( path.directory() );
  vfs::Path rpath = dir.find( path.baseName(), vfs::Path::ignoreCase );

  if( !rpath.exist() )
    return;

  _d->smkfile = smk_open_file( rpath.toString().c_str(), SMK_MODE_MEMORY );
  if( _d->smkfile != NULL )
  {
    smk_info_all( _d->smkfile, &_d->currentFrame, &_d->frameCount, &_d->usecsInFrame );
    smk_info_video( _d->smkfile, &_d->smkfileWidth, &_d->smkfileHeight, NULL );
    smk_info_audio( _d->smkfile, &_d->a_trackmask, _d->a_channels, _d->a_depth, _d->a_rate);

    Logger::warning( "Opened file {0}\nWidth: {1}\nHeight: {2}\nFrames: {3}\nFPS: {4}\n", path.toCString(),
                     _d->smkfileWidth, _d->smkfileHeight, _d->frameCount, 1000000.0 / _d->usecsInFrame );

    smk_enable_video( _d->smkfile, 1 );

    /* process first frame */
    smk_first( _d->smkfile );
    _d->updatePallete();
    _d->image_data = smk_get_video(_d->smkfile);

    _d->lastFrameTime = DateTime::elapsedTime();
    _d->needUpdateTexture = true;

    if( _d->mode == SmkViewer::video )
    {
      setWidth( _d->smkfileWidth );
      setHeight( _d->smkfileHeight );
    }
  }
}

Signal0<>& SmkViewer::onFinish() { return _d->onFinishSignal; }

SmkViewer::SmkViewer(Widget* parent, const Rect& rectangle, Mode mode)
: Widget( parent, -1, rectangle ), _d( new Impl )
{
  _d->mode = mode;
  _d->needUpdateTexture = true;
  #ifdef DEBUG
    setDebugName( TEXT(SmkViewer) );
  #endif
}

void SmkViewer::Impl::updateTexture( gfx::Engine& painter, const Size& size )
{
  Size imageSize = size;

  if( background.isValid() && background.size() != imageSize )
  {
    background = Picture();
  }

  if( !background.isValid() )
  {
    background = Picture( imageSize, 0, true );
  }

  unsigned int* pixels = background.lock();
  unsigned int bw = background.width();

  Size safe( math::min<unsigned int>( background.width(), smkfileWidth ),
             math::min<unsigned int>( background.height(), smkfileHeight ) );
  if( smkfile )
  {
    for( int i = safe.height() - 1; i >= 0; i--)
    {
      for( int j = 0; j < safe.width(); j++ )
      {
        unsigned char index = image_data[i * smkfileWidth + j];
        unsigned int* bufp32;
        bufp32 = pixels + i * bw + j;
        *bufp32 = colors[ index ];
      }
    }
  }
  background.unlock();
  background.update();
}

void SmkViewer::Impl::updatePallete()
{
  pallete = smk_get_palette( smkfile );

  for( int i = 0; i < 256; i++)
  {
    int c = ( 0xff000000 + (pallete[(i * 3) + 2])
              + (pallete[(i * 3) + 1]<<8)
              + (pallete[(i * 3)]<<16) );

    colors[ i ] = c;
  }
}

void SmkViewer::Impl::nextFrame()
{
  /* get frame number */
  if( currentFrame == frameCount )
    return;

  smk_info_all(smkfile, &currentFrame, NULL, NULL);

  if( currentFrame+1 == frameCount )
  {
    currentFrame++;
    emit onFinishSignal();
  }

  Logger::warning( " -> Frame {}...", currentFrame );

  smk_next(smkfile);
}

//! destructor
SmkViewer::~SmkViewer()
{
  if( _d->smkfile )
  {
    smk_close( _d->smkfile );
  }
}

//! draws the element and its children
void SmkViewer::draw(gfx::Engine& painter )
{
  if ( !visible() )
    return;

  // draw background
  if( _d->background.isValid() )
  {
    painter.draw( _d->background, _d->background.originRect(), absoluteRect(), &absoluteClippingRectRef() );
  }

  Widget::draw( painter );
}

void SmkViewer::_finalizeResize() { _d->needUpdateTexture = true;}

}//end namespace gui
