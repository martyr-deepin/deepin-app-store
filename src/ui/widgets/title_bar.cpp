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

#include "ui/widgets/title_bar.h"

#include <QDebug>
#include <QTimer>
#include <QDir>
#include <QPainter>
#include <QVariantMap>
#include <DThemeManager>

#include "ui/widgets/search_edit.h"
#include "ui/widgets/user_menu.h"
#include "base/consts.h"

namespace dstore
{

TitleBar::TitleBar(bool support_sign_in, QWidget *parent) : QFrame(parent)
{
    this->setObjectName("TitleBar");
    this->initUI(support_sign_in);
    this->initConnections();
}

TitleBar::~TitleBar()
{
}

QString TitleBar::getSearchText() const
{
    QString text = search_edit_->text();
    return text.remove('\n').remove('\r').remove("\r\n");
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if(event->type() == QEvent::MouseButtonPress){
        Q_EMIT titlePressed();
    }else if (event->type() == QEvent::MouseButtonDblClick) {
        Q_EMIT titleDoubleClicked();
    }
}

void TitleBar::setBackwardButtonActive(bool active)
{
    back_button_->setEnabled(active);
}

void TitleBar::setForwardButtonActive(bool active)
{
    forward_button_->setEnabled(active);
}

void TitleBar::setUserInfo(const QVariantMap &info)
{
    user_name_ = info.value("name").toString();
    auto name = info.value("nickname").toString();
    if (name.isEmpty()) {
        name = user_name_;
    }
    user_menu_->setUsername(name);

    if (user_name_.isEmpty()) {
        avatar_button_->setObjectName("AvatarButton");
        avatar_button_->setIcon(QIcon::fromTheme("deepin-app-store_login_normal"));
        return;
    }

    QDir cache_dir(dstore::GetCacheDir());
    auto avatarPath = cache_dir.filePath("avatar.png");

    avatar_button_->setObjectName("AvatarButtonUser");
    auto base64Data = QByteArray::fromStdString(info.value("profile_image").toString().toStdString());
    auto imageData = QByteArray::fromBase64(base64Data);
    auto image = QImage::fromData(imageData);
    saveUserAvatar(image, avatarPath);
    avatar_button_->setIcon(QIcon::fromTheme(avatarPath));
//    auto style = QString("#AvatarButtonUser {border-image: url(%1);}").arg(avatarPath);
//    avatar_button_->setStyleSheet(style);
}

void TitleBar::saveUserAvatar(const QImage &image, const QString &filePath)
{
    QSize sz = avatar_button_->size();

    QPixmap maskPixmap(sz);
    maskPixmap.fill(Qt::transparent);
    QPainterPath path;
    path.addEllipse(QRectF(0, 0, sz.width(), sz.height()));
    QPainter maskPainter(&maskPixmap);
    maskPainter.setRenderHint(QPainter::Antialiasing);
    maskPainter.setPen(QPen(Qt::white, 1));
    maskPainter.fillPath(path, QBrush(Qt::white));

    QPainter::CompositionMode mode = QPainter::CompositionMode_SourceIn;
    QImage contentImage = QImage(sz, QImage::Format_ARGB32_Premultiplied);
    QPainter contentPainter(&contentImage);
    contentPainter.setCompositionMode(QPainter::CompositionMode_Source);
    contentPainter.fillRect(contentImage.rect(), Qt::transparent);
    contentPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    contentPainter.drawImage(0, 0, maskPixmap.toImage());
    contentPainter.setCompositionMode(mode);
    contentPainter.drawImage(0, 0, image.scaled(avatar_button_->size(),
                             Qt::KeepAspectRatio,
                             Qt::SmoothTransformation));
    contentPainter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
    contentPainter.end();
    contentImage = contentImage.scaled(avatar_button_->size());
    contentImage.save(filePath);

    //QPixmap pixmap = QPixmap::fromImage(contentImage);

//    QPalette palette;
//    palette.setBrush(avatar_button_->backgroundRole(),
//                     QBrush(pixmap));

    avatar_button_->setFlat(true);
    avatar_button_->setAutoFillBackground(true);
//    avatar_button_->setPalette(palette);
}

void TitleBar::refreshAvatar()
{
    if (user_name_.isEmpty()) {
        return;
    }

    QDir cache_dir(dstore::GetCacheDir());
    auto avatarPath = cache_dir.filePath("avatar.png");
    auto style = QString("#AvatarButtonUser {border-image: url(%1);}").arg(avatarPath);
    avatar_button_->setStyleSheet(style);
}

void TitleBar::initConnections()
{
    connect(back_button_, &Dtk::Widget::DButtonBoxButton::clicked,
            this, &TitleBar::backwardButtonClicked);
    connect(forward_button_, &Dtk::Widget::DButtonBoxButton::clicked,
            this, &TitleBar::forwardButtonClicked);
    connect(search_edit_, &SearchEdit::textChanged,
            this, &TitleBar::onSearchTextChanged);
    connect(search_edit_, &SearchEdit::focusChanged,
            this, &TitleBar::focusChanged);
    connect(search_edit_, &SearchEdit::downKeyPressed,
            this, &TitleBar::downKeyPressed);
    connect(search_edit_, &SearchEdit::enterPressed,
            this, &TitleBar::enterPressed);
    connect(search_edit_, &SearchEdit::upKeyPressed,
            this, &TitleBar::upKeyPressed);

    connect(avatar_button_, &DIconButton::clicked,
    this, [&]() {
        if (user_name_.isEmpty()) {
            Q_EMIT loginRequested(true);
        } else {
            auto x = avatar_button_->rect().left();
            auto y = avatar_button_->rect().bottom() + 10;
            user_menu_->popup(avatar_button_->mapToGlobal(QPoint(x, y)));
        }
    });

    connect(avatar_button_, &DIconButton::pressed,
            this, [&]() {
        if (user_name_.isEmpty()) {
            avatar_button_->setIcon(QIcon::fromTheme("deepin-app-store_login_press"));
        }
    });
    connect(avatar_button_, &DIconButton::released,
            this, [&]() {
        if (user_name_.isEmpty()) {
            avatar_button_->setIcon(QIcon::fromTheme("deepin-app-store_login_normal"));
        }
    });

    connect(user_menu_, &UserMenu::requestLogout,
    this, [&] {
        Q_EMIT this->loginRequested(false);
        avatar_button_->setIcon(QIcon::fromTheme("deepin-app-store_login_normal"));
    });
    connect(user_menu_, &UserMenu::commentRequested,
            this, &TitleBar::commentRequested);
    /*connect(user_menu_, &UserMenu::requestDonates,
            this, &TitleBar::requestDonates);*/
    connect(user_menu_, &UserMenu::requestApps,
            this, &TitleBar::requestApps);
}

void TitleBar::initUI(bool support_sign_in)
{
    back_button_ = new DButtonBoxButton(DStyle::SP_ArrowLeave);
    back_button_->setDisabled(true);
    back_button_->setFixedSize(36, 36);

    forward_button_ = new DButtonBoxButton(DStyle::SP_ArrowEnter);
    forward_button_->setDisabled(true);
    forward_button_->setFixedSize(36, 36);

    back_button_->setShortcut(Qt::Key_Left);
    forward_button_->setShortcut(Qt::Key_Right);

    QList<DButtonBoxButton *> buttonList;
    buttonList << back_button_ << forward_button_;

    buttonBox = new Dtk::Widget::DButtonBox();
    buttonBox->setButtonList(buttonList, false);
    buttonBox->setFocusPolicy(Qt::NoFocus);

    QSize avatar_button_size(20,20);
    avatar_button_ = new DIconButton(this);

    QPalette palette;
    palette.setBrush(avatar_button_->backgroundRole(),Qt::transparent);
    palette.setColor(avatar_button_->backgroundRole(),Qt::transparent);

    avatar_button_->setPalette(palette);
    avatar_button_->setFixedSize(20, 20);
    avatar_button_->setAutoFillBackground(true);
    avatar_button_->setContextMenuPolicy(Qt::CustomContextMenu);
    avatar_button_->setIcon(QIcon::fromTheme("deepin-app-store_login_normal"));
    avatar_button_->setFlat(true);
    avatar_button_->setIconSize(avatar_button_size);

    user_menu_ = new UserMenu();

    search_edit_ = new SearchEdit();
    search_edit_->setObjectName("SearchEdit");
    search_edit_->setFixedWidth(300);
    search_edit_->setPlaceHolder(QObject::tr("Search"));

    QHBoxLayout *main_layout = new QHBoxLayout();
    main_layout->setContentsMargins(0, 0, 0, 0);
    //left
    main_layout->addSpacing(20);//Keep the back and forward buttons spaced from the chart
    main_layout->addWidget(buttonBox,0,Qt::AlignLeft);
    main_layout->addSpacing(50);//Keep search box centered
    //center
    main_layout->addStretch();
    main_layout->addWidget(search_edit_,0,Qt::AlignCenter);
    main_layout->addStretch();
    //right
    main_layout->addWidget(avatar_button_,0,Qt::AlignRight);
    main_layout->addSpacing(260);

    this->setLayout(main_layout);
    this->setAttribute(Qt::WA_TranslucentBackground, true);

    Dtk::Widget::DThemeManager::instance()->registerWidget(avatar_button_, "dstore--TitleBar");
    Dtk::Widget::DThemeManager::instance()->registerWidget(this);

    if (!support_sign_in) {
        avatar_button_->hide();
    }
}

void TitleBar::onSearchTextChanged()
{
    emit this->searchTextChanged(search_edit_->text());
}

}  // namespace dstore
