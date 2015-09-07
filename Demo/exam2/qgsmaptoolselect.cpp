/**************************选择图层要素工具*********************
// 编写:陈恒
// 日期:2011年3月29日
//公司:甘肃目标信息科技有限公司
*****************************************************************/
#include "qgsmaptoolselect.h"



//构造和析构函数
QgsMapToolSelect::QgsMapToolSelect(QgsMapCanvas *Mapcanvas):QgsMapTool(Mapcanvas)
{
   pLayer=NULL;
   mCursor=Qt::ArrowCursor;
   mCanvas=Mapcanvas;
   StatusFlag=true;
}

QgsMapToolSelect::~QgsMapToolSelect(void)
{
}
//设置当前被选择(活动)的图层
void QgsMapToolSelect::SetSelectLayer(QgsVectorLayer *Layer)
{
   pLayer=Layer;
}

//鼠标按钮释放时,选择包含鼠标位置的图元
void QgsMapToolSelect::canvasReleaseEvent(QMouseEvent * e )
{
   if(mCanvas==NULL){
	   return;
   }
   if(pLayer==NULL){
	   QMessageBox::about(mCanvas,QString::fromLocal8Bit("警告"),QString::fromLocal8Bit("请选择图层"));
	   return;
   }
   if(StatusFlag==false){
	   return;
   }
   //得到产生事件的按钮信息
   Qt::MouseButton mButton=e->button();
   //如果不是左按钮返回
   if(mButton!=Qt::MouseButton::LeftButton){
	   return;
   }
   //得到鼠标指针的位置
   QPoint pos=e->pos();
   //定义QgsRubberBand对象
   QgsRubberBand rubberBand(mCanvas,true);
   QRect selectRect(0,0,0,0);
   //设置鼠标位置区域
   ExpandSelectRangle(selectRect,pos);
   //将鼠标位置区域转换成地图坐标
   SetRubberBand(selectRect,&rubberBand );
   //将QgsRubberBand对象转换为几何对象，根据该几何对象在图层中选择特征
   QgsGeometry* selectGeom=rubberBand.asGeometry();
   if(!selectGeom){
	   return;
   }
   //确定是否按下ctrl键
   bool doDifference=e->modifiers()&Qt::ControlModifier ? true:false;
   //在图层中选择最靠近几何对象的特征
   SetSelectFeatures(selectGeom,false,doDifference,true);
   //SetSelectFeatures(selectGeom,doDifference);
   delete selectGeom;
   rubberBand.reset(true);
}
//提取鼠标位置一定范围作为选择区域
void QgsMapToolSelect::ExpandSelectRangle(QRect &Rect,QPoint Point)
{
   int boxSize=0;
   //如果图层不是面图元类型
   if(pLayer->geometryType()!=QGis::Polygon){
       boxSize=5;
   }
   else{
	   boxSize=1;
   }
   //设置选择区域
   Rect.setLeft(Point.x()-boxSize);
   Rect.setRight(Point.x()+boxSize);
   Rect.setTop(Point.y()-boxSize);
   Rect.setBottom(Point.y()+boxSize);
}
//将指定的设备坐标区域转换成地图坐标区域
void QgsMapToolSelect::SetRubberBand(QRect &selectRect,QgsRubberBand *pRubber)
{
    //得到当前坐标变换对象
	const QgsMapToPixel* transform=mCanvas->getCoordinateTransform();
    //将区域设备坐标转换成地图坐标
	QgsPoint ll=transform->toMapCoordinates(selectRect.left(),selectRect.bottom());
    QgsPoint ur = transform->toMapCoordinates(selectRect.right(),selectRect.top());
    pRubber->reset(true );
    //将区域的4个角点添加到QgsRubberBand对象中
	pRubber->addPoint(ll,false );
    pRubber->addPoint(QgsPoint(ur.x(), ll.y()),false );
    pRubber->addPoint(ur,false );
    pRubber->addPoint(QgsPoint( ll.x(), ur.y() ),true );
}
//选择几何特征
//selectGeometry:选择特征的选择几何体
//doContains：选择的特征是否包含在选择几何体内部
//singleSelect：仅仅选择和选择几何体最靠近的特征
void QgsMapToolSelect::SetSelectFeatures(QgsGeometry *selectGeometry,bool doContains,
										 bool doDifference,bool singleSelect) 
{
    //如果选择几何体不是多边形
    if(selectGeometry->type()!=QGis::Polygon){
        QMessageBox::about(mCanvas,QString::fromLocal8Bit("警告"),QString::fromLocal8Bit("请选择多边形图层"));
      return;
    }
    QgsGeometry selectGeomTrans(*selectGeometry);
    //设定选择几何体的坐标系和图层的坐标系一致
	if(mCanvas->mapRenderer()->hasCrsTransformEnabled()){
       try{
           //将地图绘板坐标系变换到图层坐标系
           //QgsCoordinateTransform ct(mCanvas->mapRenderer()->destinationSrs(),pLayer->crs());
            QgsCoordinateTransform ct(mCanvas->mapRenderer()->destinationCrs(),pLayer->crs());
           //设定几何体的坐标系和图层坐标系一致
		   selectGeomTrans.transform(ct);
       }
       //对于异常点抛出异常
	   catch(QgsCsException &cse){
          Q_UNUSED(cse);
          //catch exception for 'invalid' point and leave existing selection unchanged
          QMessageBox::warning(mCanvas, QObject::tr("CRS Exception"),
                            QObject::tr( "Selection extends beyond layer's coordinate system." ) );
		  return;
       }
    }
    //设置光标
	//QApplication::setOverrideCursor(Qt::WaitCursor);
    //选择和选择几何体相交或在几何体内部的特征
    //pLayer->select(QgsAttributeList(),selectGeomTrans.boundingBox(),true,true);//fancy del
    QgsRectangle queryBox= selectGeomTrans.boundingBox();
    pLayer->select(queryBox,true);
	int nh=pLayer->selectedFeatureCount();

    qDebug()<<"---selected " << nh << " features";
    //fancy debug
    // QMessageBox::about(mCanvas,QString::fromLocal8Bit("警告"),QString::fromLocal8Bit("selected count:") + QString(nh));

    QgsFeatureIds newSelectedFeatures;
    QgsFeature f;
    int closestFeatureId=0;
    bool foundSingleFeature=false;
    double closestFeatureDist=std::numeric_limits<double>::max();
	//得到当前选择的特征
    QgsFeatureList selectedFs = pLayer->selectedFeatures(); //fancy add

    try{
    //while(pLayer->nextFeature(f)){
    for(int i=0; i< selectedFs.count();i++){  //fancy add
       f = selectedFs.at(i);
       qDebug()<<"---find " << f.attributes()[0].toString();
       QgsGeometry* g=f.geometry();
	   //g是否包含在selectGeomTrans几何体内部
	   if(doContains && !selectGeomTrans.contains(g)){
           continue;
       }
       if(singleSelect){ //选择和几何体最靠近的特征
          foundSingleFeature=true;
          //计算两个几何体之间的距离
		  double distance=g->distance(selectGeomTrans);
          if(distance<=closestFeatureDist){
              closestFeatureDist=distance;
              //计算出最靠近选择几何体特征的id
			  closestFeatureId=f.id();
          }
       }
       else{ //存储符合要求的特征id
          newSelectedFeatures.insert(f.id());
       }
   }
   //确定和选择几何体最靠近特征的id
   if(singleSelect && foundSingleFeature){
       newSelectedFeatures.insert(closestFeatureId);
   }
   //如果按下ctrl键,选择多个特征
   if(doDifference){
       //得到所有选择特征的id
	   layerSelectedFeatures=pLayer->selectedFeaturesIds();
       QgsFeatureIds::const_iterator i=newSelectedFeatures.constEnd();
       while(i!=newSelectedFeatures.constBegin()){
           --i;
		   if(layerSelectedFeatures.contains(*i)){
               layerSelectedFeatures.remove( *i );
           }
           else{
               layerSelectedFeatures.insert( *i );
           }
       }
  }
  else{
      layerSelectedFeatures=newSelectedFeatures;
  }

  //设定选择的特征
  pLayer->setSelectedFeatures(layerSelectedFeatures);
    }catch(...)
    {
        QMessageBox::warning(mCanvas, QObject::tr("Select Exception"),
                         QString("inner exception!") );
    }
  //QApplication::restoreOverrideCursor();
}
//选择几何特征,用于选择面状几何特征
//selectGeometry:选择几何体
void QgsMapToolSelect::SetSelectFeatures(QgsGeometry *selectGeometry,bool doDifference) 
{
    //如果选择几何体不是多边形
    if(selectGeometry->type()!=QGis::Polygon){
      return;
    }
    QgsGeometry selectGeomTrans(*selectGeometry);
    //设定选择几何体的坐标系和图层的坐标系一致
	if(mCanvas->mapRenderer()->hasCrsTransformEnabled()){
       try{
           //将地图绘板坐标系变换到图层坐标系
           QgsCoordinateTransform ct(mCanvas->mapRenderer()->destinationCrs(),pLayer->crs());
           //设定几何体的坐标系和图层坐标系一致
		   selectGeomTrans.transform(ct);
       }
       //对于异常点抛出异常
	   catch(QgsCsException &cse){
          Q_UNUSED(cse);
          //catch exception for 'invalid' point and leave existing selection unchanged
          QMessageBox::warning(mCanvas, QObject::tr("CRS Exception"),
                            QObject::tr( "Selection extends beyond layer's coordinate system." ) );
		  return;
       }
    }
    //设置光标
	//QApplication::setOverrideCursor(Qt::WaitCursor);
    //选择和选择几何体相交或在几何体内部的特征
    QgsRectangle queryBox= selectGeomTrans.boundingBox();
    pLayer->select(queryBox,true);
	int nh=pLayer->selectedFeatureCount();
    QgsFeatureIds newSelectedFeatures;
    QgsFeature f;
    
	int  p=0;
    QgsFeatureList selectedFs = pLayer->selectedFeatures(); //fancy add

    //while(pLayer->nextFeature(f)){
    for(int i=0; i< selectedFs.count();i++){  //fancy add
       f = selectedFs.at(i);
		p++;
       QgsGeometry* g=f.geometry();
       //选择的特征是否包含在选择几何体的内部
	   //如果g包含selectGeomTrans，返回true
	   if(!g->contains(&selectGeomTrans)){
           continue;
       }
       //存储符合条件图层特征id
	   newSelectedFeatures.insert(f.id());
   }
   QgsFeatureIds layerSelectedFeatures;
   //如果按下ctrl键，可以选择多个特征
   if(doDifference){
       layerSelectedFeatures=pLayer->selectedFeaturesIds();
       QgsFeatureIds::const_iterator i = newSelectedFeatures.constEnd();
       while( i != newSelectedFeatures.constBegin()){
           --i;
          if( layerSelectedFeatures.contains( *i ) )
          {
             layerSelectedFeatures.remove( *i );
          }
          else
          {
             layerSelectedFeatures.insert( *i );
          }
       }
  }
  else{
     layerSelectedFeatures=newSelectedFeatures;
  }
  //设定选择的特征
  pLayer->setSelectedFeatures(layerSelectedFeatures);
  //QApplication::restoreOverrideCursor();
}
//设定工具状态
void QgsMapToolSelect::SetEnable(bool flag)
{
	StatusFlag=flag;
	if(StatusFlag){
		mCursor=Qt::CrossCursor;
	}
	else{
		mCursor=Qt::ArrowCursor;
	}
}
