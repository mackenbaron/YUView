/*  This file is part of YUView - The YUV player with advanced analytics toolset
*   <https://github.com/IENT/YUView>
*   Copyright (C) 2015  Institut für Nachrichtentechnik, RWTH Aachen University, GERMANY
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 3 of the License, or
*   (at your option) any later version.
*
*   In addition, as a special exception, the copyright holders give
*   permission to link the code of portions of this program with the
*   OpenSSL library under certain conditions as described in each
*   individual source file, and distribute linked combinations including
*   the two.
*   
*   You must obey the GNU General Public License in all respects for all
*   of the code used other than OpenSSL. If you modify file(s) with this
*   exception, you may extend this exception to your version of the
*   file(s), but you are not obligated to do so. If you do not wish to do
*   so, delete this exception statement from your version. If you delete
*   this exception statement from all source files in the program, then
*   also delete it here.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "playlistItemDifference.h"

#include <QPainter>

// Activate this if you want to know when which difference is loaded
#define PLAYLISTITEMDIFFERENCE_DEBUG_LOADING 0
#if PLAYLISTITEMDIFFERENCE_DEBUG_LOADING && !NDEBUG
#define DEBUG_DIFF qDebug
#else
#define DEBUG_DIFF(fmt,...) ((void)0)
#endif

playlistItemDifference::playlistItemDifference()
  : playlistItemContainer("Difference Item")
{
  setIcon(0, convertIcon(":img_difference.png"));
  // Enable dropping for difference objects. The user can drop the two items to calculate the difference from.
  setFlags(flags() | Qt::ItemIsDropEnabled);

  // For a difference item, only 2 items are allowed.
  maxItemCount = 2;
  frameLimitsMax = false;
  isDifferenceLoading = false;
  isDifferenceLoadingToDoubleBuffer = false;

  // The text that is shown when no difference can be drawn
  infoText = "Please drop two video item's onto this difference item to calculate the difference.";

  connect(&difference, &videoHandlerDifference::signalHandlerChanged, this, &playlistItemDifference::signalItemChanged);
  connect(&difference, &videoHandlerDifference::signalCacheCleared, this, &playlistItem::signalItemCacheCleared);
}

/* For a difference item, the info list is just a list of the names of the
 * child elements.
 */
infoData playlistItemDifference::getInfo() const
{
  infoData info("Difference Info");

  if (childList.count() >= 1)
    info.items.append(infoItem(QString("File 1"), childList[0]->getName()));
  if (childList.count() >= 2)
    info.items.append(infoItem(QString("File 2"), childList[1]->getName()));

  // Report the position of the first difference in coding order
  difference.reportFirstDifferencePosition(info.items);

  // Report MSE
  for (int i = 0; i < difference.differenceInfoList.length(); i++)
  {
    infoItem p = difference.differenceInfoList[i];
    info.items.append(p);
  }
    
  return info;
}

void playlistItemDifference::drawItem(QPainter *painter, int frameIdx, double zoomFactor, bool drawRawData)
{
  DEBUG_DIFF("playlistItemDifference::drawItem frameIdx %d %s", frameIdx, childLlistUpdateRequired ? "childLlistUpdateRequired" : "");
  if (childLlistUpdateRequired)
  {
    updateChildList();
    updateChildItems();
  }

  if (!difference.inputsValid())
    // Draw the emptyText
    playlistItem::drawItem(painter, frameIdx, zoomFactor, drawRawData);
  else
    // draw the videoHandler
    difference.drawFrame(painter, frameIdx, zoomFactor, drawRawData);
}

QSize playlistItemDifference::getSize() const
{ 
  if (!difference.inputsValid())
  {
    // Return the size of the empty text.
    return playlistItemContainer::getSize();
  }
  
  return difference.getFrameSize(); 
}

void playlistItemDifference::createPropertiesWidget()
{
  // Absolutely always only call this once
  assert(!propertiesWidget);

  preparePropertiesWidget(QStringLiteral("playlistItemDifference"));

  // On the top level everything is layout vertically
  QVBoxLayout *vAllLaout = new QVBoxLayout(propertiesWidget.data());

  QFrame *line = new QFrame;
  line->setObjectName(QStringLiteral("line"));
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);

  // First add the parents controls (first video controls (width/height...) then YUV controls (format,...)
  vAllLaout->addLayout(difference.createFrameHandlerControls(true));
  vAllLaout->addWidget(line);
  vAllLaout->addLayout(difference.createDifferenceHandlerControls());

  // Insert a stretch at the bottom of the vertical global layout so that everything
  // gets 'pushed' to the top
  vAllLaout->insertStretch(3, 1);
}

void playlistItemDifference::updateChildItems()
{
  // Let's find out if our child item's changed.
  frameHandler *childVideo0 = nullptr;
  frameHandler *childVideo1 = nullptr;

  if (childList.count() >= 1)
    childVideo0 = childList[0]->getFrameHandler();
  if (childList.count() >= 2)
    childVideo1 = childList[1]->getFrameHandler();

  difference.setInputVideos(childVideo0, childVideo1);

  // Update the frame range
  startEndFrame = getStartEndFrameLimits();
}

void playlistItemDifference::savePlaylist(QDomElement &root, const QDir &playlistDir) const
{
  QDomElementYUView d = root.ownerDocument().createElement("playlistItemDifference");

  // Append the indexed item's properties
  playlistItem::appendPropertiesToPlaylist(d);

  playlistItemContainer::savePlaylistChildren(d, playlistDir);

  root.appendChild(d);
}

playlistItemDifference *playlistItemDifference::newPlaylistItemDifference(const QDomElementYUView &root)
{
  playlistItemDifference *newDiff = new playlistItemDifference();

  // Load properties from the parent classes
  playlistItem::loadPropertiesFromPlaylist(root, newDiff);

  // The difference might just have children that have to be added. After adding the children don't forget
  // to call updateChildItems().
    
  return newDiff;
}

ValuePairListSets playlistItemDifference::getPixelValues(const QPoint &pixelPos, int frameIdx)
{
  ValuePairListSets newSet;

  if (childList.count() >= 1)
    newSet.append("Item A", childList[0]->getFrameHandler()->getPixelValues(pixelPos, frameIdx));

  if (childList.count() >= 2)
  {
    newSet.append("Item B", childList[1]->getFrameHandler()->getPixelValues(pixelPos, frameIdx));
    newSet.append("Diff (A-B)", difference.getPixelValues(pixelPos, frameIdx));
  }

  return newSet;
}

void playlistItemDifference::loadFrame(int frameIdx, bool playing, bool loadRawData) 
{
  Q_UNUSED(playing);

  auto state = difference.needsLoading(frameIdx, loadRawData);
  if (state == LoadingNeeded)
  {
    // Load the requested current frame
    DEBUG_DIFF("playlistItemDifference::loadFrame loading difference for frame %d", frameIdx);
    isDifferenceLoading = true;
    difference.loadFrame(frameIdx);
    isDifferenceLoading = false;
    emit signalItemChanged(true);
  }
  
  if (playing && (state == LoadingNeeded || state == LoadingNeededDoubleBuffer))
  {
    // Load the next frame into the double buffer
    int nextFrameIdx = frameIdx + 1;
    if (nextFrameIdx <= startEndFrame.second)
    {
      DEBUG_DIFF("playlistItemDifference::loadFrame loading difference into double buffer %d %s", nextFrameIdx, playing ? "(playing)" : "");
      isDifferenceLoadingToDoubleBuffer = true;
      difference.loadFrame(nextFrameIdx, true);
      isDifferenceLoadingToDoubleBuffer = false;
      emit signalItemDoubleBufferLoaded();
    }
  }
}