/**************************选择图层要素工具*********************
// 编写:陈恒
// 公司:甘肃目标信息科技有限公司
// 日期:2011年3月29日
*****************************************************************/
#ifndef QGSMAPTOOLSELECT_H
#define QGSMAPTOOLSELECT_H

//Qt头文件
#include <QMouseEvent>
#include <QPoint>
#include <QRect>
#include <QMessageBox>
#include <QApplication>

#include <limits>

//Qgis头文件
#include <qgsmaptool.h>
#include <qgsmapcanvas.h>
#include <qgsvectorlayer.h>
#include <qgsrubberband.h>
#include "qgsmaptooldrawline.h"
#include "qgsmaptooltextannotation.h"
#include "qgsmaptoolsvgannotation.h"
#include "qgsmaptoolfeatureaction.h"
#include "qgsmaptoolselect.h"
#include "qgsmaptoolselectfreehand.h"
#include "qgsmaptoolselectradius.h"
#include "qgsmaptoolselectrectangle.h"
#include "qgsmaptoolselectpolygon.h"

class QgsMapToolSelect :
	public QgsMapTool
{
    Q_OBJECT;
public:
	QgsMapToolSelect(QgsMapCanvas *);
	~QgsMapToolSelect(void);
public:
	//设置当前被选择(活动)的图层
	void SetSelectLayer(QgsVectorLayer *);
    //重载鼠标释放事件函数
	virtual void canvasReleaseEvent(QMouseEvent * e);
	//设定工具状态
	void SetEnable(bool);
private:
    QgsVectorLayer* pLayer;
	QgsFeatureIds layerSelectedFeatures;
	bool StatusFlag;
private:
    //提取鼠标位置一定范围作为选择区域
	void ExpandSelectRangle(QRect &Rect,QPoint Point);
	//将指定的设备坐标区域转换成地图坐标区域
	void SetRubberBand(QRect &selectRect,QgsRubberBand *);
	//选择图层特征
    void SetSelectFeatures(QgsGeometry *,bool,bool,bool);
	void SetSelectFeatures(QgsGeometry *,bool);
};
#endif
