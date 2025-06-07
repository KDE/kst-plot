/***************************************************************************
                netcdf_source.h  -  netCDF data source reader
                             -------------------
    begin                : 28/01/2005
    copyright            : (C) 2004 Nicolas Brisset <nicodev@users.sourceforge.net>
    email                : kst@kde.org
    modified             : 03/16/05 by K. Scott
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef NETCDFSOURCE_H
#define NETCDFSOURCE_H

#include "datasource.h"
#include "dataplugin.h"

#include <netcdf.h>
#include <netcdfcpp.h>


class DataInterfaceNetCdfScalar;
class DataInterfaceNetCdfString;
class DataInterfaceNetCdfVector;
class DataInterfaceNetCdfMatrix;

class NetcdfSource : public Kst::DataSource {
  public:
    NetcdfSource(Kst::ObjectStore *store, QSettings *cfg, const QString& filename, const QString& type, const QDomElement &element);

    ~NetcdfSource();

    bool initFile();

    Kst::Object::UpdateType internalDataSourceUpdate();


    virtual QString typeString() const;

    static const QString netcdfTypeKey();


    int readScalar(double *v, const QString& field);

    int readString(QString *stringValue, const QString& stringName);

    int readField(double *v, const QString& field, int s, int n);

    int readMatrix(double *v, const QString& field);

    int samplesPerFrame(const QString& field);

    int frameCount(const QString& field = QString()) const;

    QString fileType() const;

    //void save(QTextStream &ts, const QString& indent = QString());

    bool isEmpty() const;

    void  reset();

  private:
    QMap<QString, int> _frameCounts;

    int _maxFrameCount;
    NcFile *_ncfile;

    // we must hold an NcError to overwrite the exit-on-error behaviour of netCDF
    NcError _ncErr;

    QMap<QString, QString> _strings;

    // TODO remove friend
    QStringList _scalarList;
    QStringList _fieldList;
    QStringList _matrixList;
    //QStringList _stringList;


    friend class DataInterfaceNetCdfScalar;
    friend class DataInterfaceNetCdfString;
    friend class DataInterfaceNetCdfVector;
    friend class DataInterfaceNetCdfMatrix;
    DataInterfaceNetCdfScalar* is;
    DataInterfaceNetCdfString* it;
    DataInterfaceNetCdfVector* iv;
    DataInterfaceNetCdfMatrix* im;
};



#endif

