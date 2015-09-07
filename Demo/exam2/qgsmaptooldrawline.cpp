/**************************绘制折线工具*********************
// 编写:陈恒
// 公司:甘肃目标信息科技有限公司
// 日期:2011年3月29日
*****************************************************************/
#include "qgsmaptooldrawline.h"

//Qgis头文件


QgsMapToolDrawLine::QgsMapToolDrawLine(QgsMapCanvas *canvas):QgsMapTool(canvas)
{
    pMapCanvas=canvas;
    //设置QgsRubberBand对象,绘制折线
	pRubBand=new QgsRubberBand(pMapCanvas);
	mColor=QColor(255,0,0);
	LineWidth=2;
	ButtonClickFlag=false;
}

QgsMapToolDrawLine::~QgsMapToolDrawLine(void)
{
	/*if(pRubBand){
	   delete pRubBand ;
	}*/
}
//重载鼠标移动事件
void QgsMapToolDrawLine::canvasMoveEvent (QMouseEvent *e)
{
	//int x,y;
    QgsPoint mPoint;
	int xc,yc;
	//如果鼠标左键没有双击
	if(!ButtonClickFlag){
		return;
	}
	pRubBand->setColor(mColor);
	pRubBand->setWidth(LineWidth);
	xc=e->x();
	yc=e->y();
	//得到当前坐标变换对象
	const QgsMapToPixel* pTransform=pMapCanvas->getCoordinateTransform();
	//转换成地图坐标
	mPoint=pTransform->toMapCoordinates(xc,yc);
	if(pRubBand->numberOfVertices()>1){
		pRubBand->removeLastPoint();
	}
	//把当前点添加到QgsRubberBand对象中用于绘制
	pRubBand->addPoint(mPoint);
}
//重载鼠标双击事件
void  QgsMapToolDrawLine::canvasDoubleClickEvent (QMouseEvent *e)
{
	QgsPoint mPoint;
	int xc,yc;
	pRubBand->setColor(mColor);
	pRubBand->setWidth(LineWidth);
	//得到当前坐标变换对象
	const QgsMapToPixel* pTransform=pMapCanvas->getCoordinateTransform();
	//得到产生事件的按钮信息
	Qt::MouseButton mButton=e->button();
	xc=e->x();
	yc=e->y();
	mPoint=pTransform->toMapCoordinates(xc,yc);
	//如果是左按钮
	if(mButton==Qt::MouseButton::LeftButton){
		ButtonClickFlag=true;
	    //把当前点添加到QgsRubberBand对象中用于绘制
		pRubBand->addPoint(mPoint);
		mPointSet.append(mPoint);
	}
	else if(mButton==Qt::MouseButton::RightButton){
		pRubBand->addPoint(mPoint); 
		mPointSet.append(mPoint);
		ButtonClickFlag=false;
		//转化为几何体
		pRubBand->asGeometry();
	}
}
//设置绘制颜色和线宽
void QgsMapToolDrawLine::SetColorAndWidth(QColor color,int nWidth)
{
    mColor=color;
	LineWidth=nWidth;
}
//重载鼠标单击击事件
void QgsMapToolDrawLine::canvasPressEvent(QMouseEvent *e) 
{
	if(ButtonClickFlag){
		return;
	}
	//得到产生事件的按钮信息
	Qt::MouseButton mButton=e->button();
	//如果是左按钮
	if(mButton==Qt::MouseButton::LeftButton){
		pRubBand->reset();
	}
}
//返回构成线段的数据点数
int QgsMapToolDrawLine::GetVertexCount()
{
	return mPointSet.size();
}
//得到点的坐标
bool QgsMapToolDrawLine::GetCoord(int index,double &x,double &y)
{
	QgsPoint mPoint;
	if(index>=mPointSet.size()){
		return true;
	}
	mPoint=mPointSet.at(index);
	x=mPoint.x();
	y=mPoint.y();
}
