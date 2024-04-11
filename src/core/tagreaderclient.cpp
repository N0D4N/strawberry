/*
 * Strawberry Music Player
 * This file was part of Clementine.
 * Copyright 2010, David Sansome <me@davidsansome.com>
 * Copyright 2019-2023, Jonas Kvinge <jonas@jkvinge.net>
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

#include "config.h"

#include <string>

#include <QtGlobal>
#include <QObject>
#include <QThread>
#include <QByteArray>
#include <QString>
#include <QImage>

#include "core/logging.h"
#include "core/workerpool.h"

#include "song.h"
#include "tagreaderclient.h"

namespace {
constexpr char kWorkerExecutableName[] = "strawberry-tagreader";
}

TagReaderClient *TagReaderClient::sInstance = nullptr;

TagReaderClient::TagReaderClient(QObject *parent) : QObject(parent), worker_pool_(new WorkerPool<HandlerType>(this)) {

  sInstance = this;
  original_thread_ = thread();

  worker_pool_->SetExecutableName(QLatin1String(kWorkerExecutableName));
  QObject::connect(worker_pool_, &WorkerPool<HandlerType>::WorkerFailedToStart, this, &TagReaderClient::WorkerFailedToStart);

}

void TagReaderClient::Start() { worker_pool_->Start(); }

void TagReaderClient::ExitAsync() {
  QMetaObject::invokeMethod(this, &TagReaderClient::Exit, Qt::QueuedConnection);
}

void TagReaderClient::Exit() {

  Q_ASSERT(QThread::currentThread() == thread());
  moveToThread(original_thread_);
  emit ExitFinished();

}

void TagReaderClient::WorkerFailedToStart() {
  qLog(Error) << "The" << kWorkerExecutableName << "executable was not found in the current directory or on the PATH.  Strawberry will not be able to read music file tags without it.";
}

TagReaderReply *TagReaderClient::IsMediaFile(const QString &filename) {

  spb::tagreader::Message message;
  spb::tagreader::IsMediaFileRequest *request = message.mutable_is_media_file_request();

  const QByteArray filename_data = filename.toUtf8();
  request->set_filename(filename_data.constData(), filename_data.length());

  return worker_pool_->SendMessageWithReply(&message);

}

TagReaderReply *TagReaderClient::ReadFile(const QString &filename) {

  spb::tagreader::Message message;
  spb::tagreader::ReadFileRequest *request = message.mutable_read_file_request();

  const QByteArray filename_data = filename.toUtf8();
  request->set_filename(filename_data.constData(), filename_data.length());

  return worker_pool_->SendMessageWithReply(&message);

}

TagReaderReply *TagReaderClient::SaveFile(const QString &filename, const Song &metadata, const SaveTypes save_types, const SaveCoverOptions &save_cover_options) {

  spb::tagreader::Message message;
  spb::tagreader::SaveFileRequest *request = message.mutable_save_file_request();

  const QByteArray filename_data = filename.toUtf8();
  request->set_filename(filename_data.constData(), filename_data.length());

  request->set_save_tags(save_types.testFlag(SaveType::Tags));
  request->set_save_playcount(save_types.testFlag(SaveType::PlayCount));
  request->set_save_rating(save_types.testFlag(SaveType::Rating));
  request->set_save_cover(save_types.testFlag(SaveType::Cover));

  if (save_cover_options.cover_filename.length() > 0) {
    const QByteArray cover_filename = save_cover_options.cover_filename.toUtf8();
    request->set_cover_filename(cover_filename.constData(), cover_filename.length());
  }
  if (save_cover_options.cover_data.length() > 0) {
    request->set_cover_data(save_cover_options.cover_data.constData(), save_cover_options.cover_data.length());
  }
  if (save_cover_options.mime_type.length() > 0) {
    const QByteArray cover_mime_type = save_cover_options.mime_type.toUtf8();
    request->set_cover_mime_type(cover_mime_type.constData(), cover_mime_type.length());
  }
  metadata.ToProtobuf(request->mutable_metadata());

  ReplyType *reply = worker_pool_->SendMessageWithReply(&message);

  return reply;

}

TagReaderReply *TagReaderClient::LoadEmbeddedArt(const QString &filename) {

  spb::tagreader::Message message;
  spb::tagreader::LoadEmbeddedArtRequest *request = message.mutable_load_embedded_art_request();

  const QByteArray filename_data = filename.toUtf8();
  request->set_filename(filename_data.constData(), filename_data.length());

  return worker_pool_->SendMessageWithReply(&message);

}

TagReaderReply *TagReaderClient::SaveEmbeddedArt(const QString &filename, const SaveCoverOptions &save_cover_options) {

  spb::tagreader::Message message;
  spb::tagreader::SaveEmbeddedArtRequest *request = message.mutable_save_embedded_art_request();

  const QByteArray filename_data = filename.toUtf8();
  request->set_filename(filename_data.constData(), filename_data.length());
  if (save_cover_options.cover_filename.length() > 0) {
    const QByteArray cover_filename = save_cover_options.cover_filename.toUtf8();
    request->set_cover_filename(cover_filename.constData(), cover_filename.length());
  }
  if (save_cover_options.cover_data.length() > 0) {
    request->set_cover_data(save_cover_options.cover_data.constData(), save_cover_options.cover_data.length());
  }
  if (save_cover_options.mime_type.length() > 0) {
    const QByteArray cover_mime_type = save_cover_options.mime_type.toUtf8();
    request->set_cover_mime_type(cover_mime_type.constData(), cover_mime_type.length());
  }

  return worker_pool_->SendMessageWithReply(&message);

}

TagReaderReply *TagReaderClient::UpdateSongPlaycount(const Song &metadata) {

  spb::tagreader::Message message;
  spb::tagreader::SaveSongPlaycountToFileRequest *request = message.mutable_save_song_playcount_to_file_request();

  const QByteArray filename_data = metadata.url().toLocalFile().toUtf8();
  request->set_filename(filename_data.constData(), filename_data.length());
  metadata.ToProtobuf(request->mutable_metadata());

  return worker_pool_->SendMessageWithReply(&message);

}

void TagReaderClient::UpdateSongsPlaycount(const SongList &songs) {

  for (const Song &song : songs) {
    TagReaderReply *reply = UpdateSongPlaycount(song);
    QObject::connect(reply, &TagReaderReply::Finished, reply, &TagReaderReply::deleteLater);
  }

}

TagReaderReply *TagReaderClient::UpdateSongRating(const Song &metadata) {

  spb::tagreader::Message message;
  spb::tagreader::SaveSongRatingToFileRequest *request = message.mutable_save_song_rating_to_file_request();

  const QByteArray filename_data = metadata.url().toLocalFile().toUtf8();
  request->set_filename(filename_data.constData(), filename_data.length());
  metadata.ToProtobuf(request->mutable_metadata());

  return worker_pool_->SendMessageWithReply(&message);

}

void TagReaderClient::UpdateSongsRating(const SongList &songs) {

  for (const Song &song : songs) {
    TagReaderReply *reply = UpdateSongRating(song);
    QObject::connect(reply, &TagReaderReply::Finished, reply, &TagReaderReply::deleteLater);
  }

}

bool TagReaderClient::IsMediaFileBlocking(const QString &filename) {

  Q_ASSERT(QThread::currentThread() != thread());

  bool ret = false;

  TagReaderReply *reply = IsMediaFile(filename);
  if (reply->WaitForFinished()) {
    ret = reply->message().is_media_file_response().success();
  }
  reply->deleteLater();

  return ret;

}

void TagReaderClient::ReadFileBlocking(const QString &filename, Song *song) {

  Q_ASSERT(QThread::currentThread() != thread());

  TagReaderReply *reply = ReadFile(filename);
  if (reply->WaitForFinished()) {
    song->InitFromProtobuf(reply->message().read_file_response().metadata());
  }
  reply->deleteLater();

}

bool TagReaderClient::SaveFileBlocking(const QString &filename, const Song &metadata, const SaveTypes save_types, const SaveCoverOptions &save_cover_options) {

  Q_ASSERT(QThread::currentThread() != thread());

  bool ret = false;

  TagReaderReply *reply = SaveFile(filename, metadata, save_types, save_cover_options);
  if (reply->WaitForFinished()) {
    ret = reply->message().save_file_response().success();
  }
  reply->deleteLater();

  return ret;

}

QByteArray TagReaderClient::LoadEmbeddedArtBlocking(const QString &filename) {

  Q_ASSERT(QThread::currentThread() != thread());

  QByteArray ret;

  TagReaderReply *reply = LoadEmbeddedArt(filename);
  if (reply->WaitForFinished()) {
    const std::string &data_str = reply->message().load_embedded_art_response().data();
    ret = QByteArray(data_str.data(), static_cast<qint64>(data_str.size()));
  }
  reply->deleteLater();

  return ret;

}

QImage TagReaderClient::LoadEmbeddedArtAsImageBlocking(const QString &filename) {

  Q_ASSERT(QThread::currentThread() != thread());

  QImage ret;

  TagReaderReply *reply = LoadEmbeddedArt(filename);
  if (reply->WaitForFinished()) {
    const std::string &data_str = reply->message().load_embedded_art_response().data();
    ret.loadFromData(QByteArray(data_str.data(), static_cast<qint64>(data_str.size())));
  }
  reply->deleteLater();

  return ret;

}

bool TagReaderClient::SaveEmbeddedArtBlocking(const QString &filename, const SaveCoverOptions &save_cover_options) {

  Q_ASSERT(QThread::currentThread() != thread());

  bool success = false;

  TagReaderReply *reply = SaveEmbeddedArt(filename, save_cover_options);
  if (reply->WaitForFinished()) {
    success = reply->message().save_embedded_art_response().success();
  }
  reply->deleteLater();

  return success;

}

bool TagReaderClient::UpdateSongPlaycountBlocking(const Song &metadata) {

  Q_ASSERT(QThread::currentThread() != thread());

  bool success = false;

  TagReaderReply *reply = UpdateSongPlaycount(metadata);
  if (reply->WaitForFinished()) {
    success = reply->message().save_song_playcount_to_file_response().success();
  }
  reply->deleteLater();

  return success;

}

bool TagReaderClient::UpdateSongRatingBlocking(const Song &metadata) {

  Q_ASSERT(QThread::currentThread() != thread());

  bool success = false;

  TagReaderReply *reply = UpdateSongRating(metadata);
  if (reply->WaitForFinished()) {
    success = reply->message().save_song_rating_to_file_response().success();
  }
  reply->deleteLater();

  return success;

}
