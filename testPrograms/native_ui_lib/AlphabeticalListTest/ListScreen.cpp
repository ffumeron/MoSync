/*
 Copyright (C) 2012 MoSync AB

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License,
 version 2, as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 MA 02110-1301, USA.
 */

/**
 * ListScreen.cpp
 *
 *  Created on: Aug 7, 2012
 *      Author: Spiridon Alexandru
 */
#include <conprint.h>
#include <wchar.h>
#include <ma.h>
#include <maassert.h>
#include <mawstring.h>
#include <mastdlib.h>
#include <NativeUI/ListViewSection.h>

#include "ListScreen.h"

/**
 * Constructor.
 */
ListScreen::ListScreen() :
	Screen(),
	mMainLayout(NULL),
	mListView(NULL)
{
	createMainLayout();

	mListView->addListViewListener(this);
}

/**
 * Destructor.
 */
ListScreen::~ListScreen()
{
	mListView->removeListViewListener(this);
}

/**
 * Creates and adds main layout to the screen.
 */
void ListScreen::createMainLayout() {
	// Create and add the main layout to the screen.
	mMainLayout = new VerticalLayout();
	Screen::setMainWidget(mMainLayout);

	mListView = new ListView(LIST_VIEW_TYPE_ALPHABETICAL);

	// the list view doesn't automatically sort its elements - the
	// developer has to handle the sorting
	for (int i = 0; i <= 4; i++)
	{
		ListViewSection* section = new ListViewSection(LIST_VIEW_SECTION_TYPE_ALPHABETICAL);
		MAUtil::String sectionTitle = "A";
		sectionTitle[0] += i;
		section->setTitle(sectionTitle);
		section->setHeaderText(sectionTitle);
		for (int j = 0; j <= 20; j++)
		{
			ListViewItem* item = new ListViewItem();
			MAUtil::String itemText = sectionTitle + "0";
			itemText[1] += j;
			item->setText(itemText);
			item->setSubtitle("subtitle");
			section->addItem(item);
		}
		mListView->addChild(section);
	}

	int result = mMainLayout->addChild(mListView);
    printf("add mListView result = %d", result);
}

/**
 * Gets the current alphabetical list view.
 * @return The current alphabetical list view.
 */
ListView* ListScreen::getAlphabeticalListView()
{
	return mListView;
}

/**
 * This method is called when a list view item is clicked.
 * @param listView The list view object that generated the event.
 * @param listViewItem The ListViewItem object that was clicked.
 */
void ListScreen::listViewItemClicked(
    ListView* listView,
    ListViewItem* listViewItem)
{
    if (listView == mListView)
    {
        printf("ListScreen: mListView event: item clicked");
    }
}
