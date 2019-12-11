/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ui/widgets/title_bar_menu.h"

#include <QDebug>

#include "services/settings_manager.h"

namespace dstore
{

TitleBarMenu::TitleBarMenu(bool support_sign_in, QWidget *parent)
    : QMenu(parent)
{

    this->initActions();
}

TitleBarMenu::~TitleBarMenu()
{

}

void TitleBarMenu::initActions()
{
    this->addAction(QObject::tr("Clear cache"),
                    this, &TitleBarMenu::clearCacheRequested);

    privacy_agreement_action_ = this->addAction(QObject::tr("Privacy Policy"));
    connect(privacy_agreement_action_, &QAction::triggered,
            this, &TitleBarMenu::privacyAgreementRequested);

    this->addSeparator();
}


}  // namespace dstore
