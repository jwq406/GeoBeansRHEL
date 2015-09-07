#ifndef EXAMP1112_H
#define EXAMP1112_H

#include <QtGui/QMainWindow>
#include "ui_examp2.h"
/*
#include <qgsmapcanvas.h>
#include <qgsmaptool.h>
#include <qgsapplication.h>
#include <qgsproviderregistry.h>
//#include <qgssinglesymbolrenderer.h>
#include <qgssinglesymbolrendererv2.h> 
#include <qgsmaplayerregistry.h>
#include <qgsvectorlayer.h>
#include <qgsmapcanvas.h>
#include <qgsmaptoolpan.h>
#include <qgsmaptoolzoom.h>
//#include <qgslegend.h>
#include <qgslegendinterface.h> //add
#include <qgssymbolv2.h>
#include <qgslabelattributes.h>
#include <qgslabel.h>
#include <qgsvectordataprovider.h>
#include <qgsrubberband.h>
//#include <qgssearchtreenode.h>
//#include <qgssearchstring.h>
#include "qgsexpression.h" //add
#include <qgsmaptoolemitpoint.h>
#include <qgsmaptoolselect.h>
#include <QListWidgetItem>

#include "qgsutility.h"
#include "qgsmaptooldrawline.h"
*/

//#include "qgsmaptoolcapture.h"
//#include "qgsmaptoolidentify.h"
//#include "qgsmaptoolselect.h"
//#include "qgsmaptoolvertexedit.h"
//#include "qgsmeasure.h"

class QgsMapCanvas;
class QgsMapTool;
class  QgsMapToolEmitPoint;
class QgsMapToolSelect;
class QgsMapToolDrawLine;
class QgsMapCanvasLayer;
class QgsMapLayerRegistry;
class QgsLegendInterface;
class QgsRubberBand;
class QgsVectorLayer;
class QgsPoint;
class QgsMessageBar;

class examp2 : public QMainWindow,public Ui_examp2Class
{
	Q_OBJECT

public:
	examp2(QWidget *parent = 0, Qt::WFlags flags = 0);
	~examp2();

     static examp2 *instance() { return smInstance; }
public slots:
   void zoomInMode();
   void zoomOutMode();
   void panMode();
   //void addLayer();
   void FullShow();
   void measrueDist(); //测量距离
   void measrueArea(); //测量面积
   void textAnno();
   void svgAnno();
   void doFeatureAction();
   //隐藏/显示居民地图层
   void EnableLayer1(bool);
   void EnableLayer2(bool);
   //注记居民地图层
   void EnableLabel1(bool);
   void MouseXY(const QgsPoint &p);
   //选择图层
   void SelectLayer(QListWidgetItem *);
   void SelectLayer_0(QListWidgetItem *);
   //选择字段
   void SelectField(QListWidgetItem *);
   //选择特征
   void SelectFeature(QListWidgetItem *);
   //选择工具选择图元
   void SelectElement();
   void DeleteElement();
   void GetAttribute();


   //! activates the rectangle selection tool
   void selectFeatures();

   //! activates the polygon selection tool
   void selectByPolygon();

   //! activates the freehand selection tool
   void selectByFreehand();

   //! activates the radius selection tool
   void selectByRadius();

   //! deselect features from all layers
   void deselectAll();

   //! select features by expression
   void selectByExpression();

   //绘制折线
   void DrawPolyLine();

   void DoOpenFly();
private:

    static examp2* smInstance;
	//Ui::examp2Class ui;
	QgsMapCanvas *pMapCanvas;
    QgsMessageBar *mInfoBar;
    QgsMessageBar *messageBar();

	//地图交互工具
	QgsMapTool *pPanTool;
    QgsMapTool *pZoomInTool;
    QgsMapTool *pZoomOutTool;
    QgsMapToolEmitPoint *pEmitPointTool;
	QgsMapToolSelect *pSelectTool;
    QgsMapTool *mMeasureDist;
    QgsMapTool *mMeasureArea;
    QgsMapTool *mTextAnnotationTool;
    QgsMapTool *mSvgAnnotationTool;
    QgsMapTool *mFeatureAction;

    //选择工具
    QgsMapTool *mSelectFeatures;
    QgsMapTool *mSelectPolygon;
    QgsMapTool *mSelectFreehand;
    QgsMapTool *mSelectRadius;
  //  QgsMaptool mSelectRectangle;

	//绘制工具
	QgsMapToolDrawLine *pDrawLineTool;

	QList<QgsMapCanvasLayer> myLayerSet;
	QgsMapLayerRegistry *pMapLayer;
    QgsLegendInterface *pLegend;
	QgsRubberBand * mpRubberBand;
	QgsVectorLayer *pSelectLayer;
	QString FieldName;

private:
	//查询图层,设置其可见性
	bool QueryLayer(QString,bool);
    void attributeTable(QgsVectorLayer *pLayer);

};

#endif // EXAMP2_H
