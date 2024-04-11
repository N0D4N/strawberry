/*
 * Strawberry Music Player
 * Copyright 2024, Jonas Kvinge <jonas@jkvinge.net>
 *
 * Strawberry is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Strawberry is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Strawberry.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <QObject>
#include <QVariant>

class Settings : public QSettings {
  Q_OBJECT

 public:
  explicit Settings(QObject *parent = nullptr);
  explicit Settings(const QString &filename, const Format format, QObject *parent = nullptr);
#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)  // Compatibility with older Qt versions
  void beginGroup(const char *prefix);
  void beginGroup(const QString &prefix);
  bool contains(const char *key) const;
  bool contains(const QString &key) const;
  QVariant value(const char *key, const QVariant &default_value = QVariant()) const;
  QVariant value(const QString &key, const QVariant &default_value = QVariant()) const;
  void setValue(const char *key, const QVariant &value);
  void setValue(const QString &key, const QVariant &value);
  int beginReadArray(const char *prefix);
  int beginReadArray(const QString &prefix);
  void beginWriteArray(const char *prefix, int size = -1);
  void beginWriteArray(const QString &prefix, int size = -1);
  void remove(const char *key);
  void remove(const QString &key);
#endif
};

#endif  // SETTINGS_H
