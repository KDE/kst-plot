/***************************************************************************
 *                                                                         *
 *   copyright : (C) 2004 The University of Toronto                        *
 *                   netterfield@astro.utoronto.ca                         *
*                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef DEBUG_H
#define DEBUG_H

#include <config.h>

#include <qdatetime.h>
#include <qpointer.h>
#include <qobject.h>
#include <qmutex.h>

#include <QThread>

#include "kstcore_export.h"

namespace Kst {

// This class has to be threadsafe
class KSTCORE_EXPORT Debug : public QObject {
  Q_OBJECT
  public:
    enum LogLevel {
      Trace     = 1,
      Notice    = 2,
      Warning   = 4,
      Error     = 8
    };
    struct LogMessage {
      QDateTime date;
      QString msg;
      LogLevel level;
    };
    static Debug *self();

    void clear();
    void log(const QString& msg, LogLevel level = Notice);
    void setLimit(bool applyLimit, int limit);
    QString text();

#define DEBUG_LOG_FUNC(X, T) static void X(const QString& msg) { self()->log(msg, T); }
    DEBUG_LOG_FUNC(error, Error);
    DEBUG_LOG_FUNC(warning, Warning);
    DEBUG_LOG_FUNC(notice, Notice);
    DEBUG_LOG_FUNC(trace, Trace);

    int logLength() const;
    QList<LogMessage> messages() const;
    Debug::LogMessage message(unsigned n) const;
    QStringList dataSourcePlugins() const;
    QString label(LogLevel level) const;
    const QString& kstRevision() const;

    int limit() const;

    bool hasNewError() const;
    void clearHasNewError();

#ifdef BENCHMARK
    QMap<QString,int>& drawCounter() { return _drawCounter; }
#endif

    void setHandler(QObject *handler);

  private:
    Debug();
    ~Debug();

    static Debug *_self;
    static void cleanup();

    QList<LogMessage> _messages;
    bool _applyLimit;
    bool _hasNewError;
    int _limit;
    mutable QMutex _lock;
#ifdef BENCHMARK
    // If this is ever public we can't do this
    QMap<QString,int> _drawCounter;
#endif
    QPointer<QObject> _handler;
    QString _kstRevision;
};


struct Sleep : QThread
{
    static void ms(int t) { QThread::msleep(t); }
};


}
#endif

// vim: ts=2 sw=2 et
