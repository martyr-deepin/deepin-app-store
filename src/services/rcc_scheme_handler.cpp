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

#include "services/rcc_scheme_handler.h"

#include <QDebug>
#include <QFileInfo>
#include <QLocale>

namespace dstore {

QString RccSchemeHandler(const QUrl& url) {
  QString path = url.path();
  const QString host = url.host();

  if (host == "web") {
    const char kAppDefaultLocalDir[] = DSTORE_WEB_DIR;

    QString lang = QLocale().name();
    lang = lang.replace(QRegExp("\\_"), "-");

    QString app_lang_dir = QString("%1/%2")
            .arg(DSTORE_WEB_DIR)
            .arg(lang);

    QString app_en_dir = QString("%1/%2")
            .arg(DSTORE_WEB_DIR)
            .arg("en-US");

    QString prefix =QString("%1%2").arg("/").arg(lang);
    path.remove(prefix);

    if (!QFileInfo::exists(app_lang_dir)) {
      app_lang_dir = app_en_dir;
      path.remove("/en-US");

      if(!QFileInfo::exists(app_en_dir)){
          app_lang_dir = kAppDefaultLocalDir;
      }
    }

    QString filepath = QString("%1%2").arg(app_lang_dir).arg(path);
    return filepath;
  } else {
    // 404 not found.
    return "";
  }
}

}  // namespace dstore
