#include "examp2.h"
#include <QMessageBox>
#include <qgsrectangle.h>
#include <qgisgui.h>
#include <QFileDialog>
#include <QListWidgetItem>
#include <QList>
#include <QCheckBox>

#include <qgsmaplayer.h>
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
#include "qgssymbollayerv2registry.h"
#include "qgsmeasuretool.h"

#include "qgsutility.h"
#include "qgsmaptooldrawline.h"
#include "qgsmaptooltextannotation.h"
#include "qgsmaptoolsvgannotation.h"
#include "qgsmaptoolfeatureaction.h"
#include "qgsmaptoolselect.h"
#include "qgsmaptoolselectfreehand.h"
#include "qgsmaptoolselectradius.h"
#include "qgsmaptoolselectrectangle.h"
#include "qgsmaptoolselectpolygon.h"
#include "qgseditorwidgetregistry.h"
#include "qgsmessagebar.h"
#include "qgsattributeactiondialog.h"
#include "qgsattributetabledialog.h"

#include "flycontrolsetting.h"





using namespace QgisGui;

const char* PLUGINS_DIR = "/opt/mygis/lib/qgis/plugins";
const char* DEMODATA_DIR = "/opt/mygis/data/vector/world";
const char* POINT_DATA="world_100_city";
const char* POLYLINE_DATA = "world_100_river";
const char* POLYGON_DATA = "world_100_country";

examp2* examp2::smInstance = 0;

examp2::examp2(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{

    if ( smInstance )
    {
      QMessageBox::critical(
        this,
        tr( "Multiple Instances of QgisApp" ),
        tr( "Multiple instances of QGIS application object detected.\nPlease contact the developers.\n" ) );
      abort();
    }

    smInstance = this;

	QString LayerPath;
	setupUi(this);
	//地图交互工具执行
	connect(qtID_Pan,SIGNAL(triggered()),this,SLOT(panMode()));
	connect(qtID_ZoomIn,SIGNAL(triggered()),this,SLOT(zoomInMode()));
	connect(qtID_ZoomOut,SIGNAL(triggered()),this,SLOT(zoomOutMode()));
    connect(qtID_FullShow,SIGNAL(triggered()),this,SLOT(FullShow()));
	connect(qtID_SelectFeature,SIGNAL(triggered()),this,SLOT(SelectElement()));
    connect(qtID_DeleFeature,SIGNAL(triggered()),this,SLOT(DeleteElement()));
	connect(qtID_GetAttribute,SIGNAL(triggered()),this,SLOT(GetAttribute()));
    connect(actionMeasureDist,SIGNAL(triggered()),this, SLOT(measrueDist()));
    connect(mActionMeasureArea,SIGNAL(triggered()),this, SLOT(measrueArea()));
    connect(mActionTextAnnotation,SIGNAL(triggered()),this, SLOT(textAnno()));
    connect(mActionSvgAnnotation,SIGNAL(triggered()),this, SLOT(svgAnno()));


    connect( mActionSelectFeatures, SIGNAL( triggered() ), this, SLOT( selectFeatures() ) );
    // connect( mActionSelectPolygon, SIGNAL( triggered() ), this, SLOT( selectByPolygon() ) );
    // connect( mActionSelectFreehand, SIGNAL( triggered() ), this, SLOT( selectByFreehand() ) );
     connect( mActionSelectRadius, SIGNAL( triggered() ), this, SLOT( selectByRadius() ) );
    // connect( mActionDeselectAll, SIGNAL( triggered() ), this, SLOT( deselectAll() ) );
    // connect( mActionSelectByExpression, SIGNAL( triggered() ), this, SLOT( selectByExpression() ) );

    connect( mActionDeselectAll, SIGNAL( triggered() ), this, SLOT( deselectAll() ) );

	//执行菜单操作
	//绘制折线
	connect(qtID_PolyLine,SIGNAL(triggered()),this,SLOT(DrawPolyLine()));
    connect(OpenFly,SIGNAL(triggered()),this,SLOT(DoOpenFly()) );
    //注册QGIS插件
    QString myPluginsDir(PLUGINS_DIR);
    QgsProviderRegistry::instance(myPluginsDir);
	//创建地图显示画板
	pMapCanvas=new QgsMapCanvas(this);
    pMapCanvas->enableAntiAliasing(true);
    //pMapCanvas->useImageToRender(false);
    pMapCanvas->setCanvasColor(QColor(155,176,227));
    pMapCanvas->freeze(false);
    pMapCanvas->setVisible(true);
    pMapCanvas->setFocus();
	//设为中心Widget
	setCentralWidget(pMapCanvas);
	connect(pMapCanvas,SIGNAL(xyCoordinates(const QgsPoint &)),this,SLOT(MouseXY(const QgsPoint &)));

    //mInfoBar = new QgsMessageBar( centralWidget() );

    // Init the editor widget types
   // QgsEditorWidgetRegistry::initEditors( pMapCanvas, mInfoBar );

	//设置选择图层时被选择图元的颜色
    //QgsRenderer::setSelectionColor(QColor(255,0,0));   fancy del 4 old version
    QgsRenderContext renderContext = QgsRenderContext::fromMapSettings(pMapCanvas->mapSettings());
    renderContext.setSelectionColor(QColor(255,0,0)); //upgrade to 2.10.0
	//创建图例管理工具
    //pLegend=new QgsLegend(pMapCanvas,LegenddockWidget);
    //horizontalLayout->addWidget(pLegend);
	//地图操作工具
	pPanTool=new QgsMapToolPan(pMapCanvas);
    pPanTool->setAction(qtID_Pan);
    pZoomInTool=new QgsMapToolZoom(pMapCanvas, FALSE); // false = in
    pZoomInTool->setAction(qtID_ZoomIn);
    pZoomOutTool=new QgsMapToolZoom(pMapCanvas, TRUE ); //true = out
    pZoomOutTool->setAction(qtID_ZoomOut);
	pEmitPointTool=new QgsMapToolEmitPoint(pMapCanvas);
	pSelectTool=new QgsMapToolSelect(pMapCanvas);
    pSelectTool->setAction(qtID_SelectFeature);
	//绘制工具
    pDrawLineTool=new QgsMapToolDrawLine(pMapCanvas);
	pDrawLineTool->setAction(qtID_PolyLine);

    mMeasureDist = new QgsMeasureTool(pMapCanvas,false); //fancy 测量工具来自 APP
    mMeasureDist->setAction(actionMeasureDist);

    mMeasureArea = new QgsMeasureTool( pMapCanvas, true /* area */ );
    mMeasureArea->setAction( mActionMeasureArea );

    mTextAnnotationTool = new QgsMapToolTextAnnotation( pMapCanvas );
    mTextAnnotationTool->setAction( mActionTextAnnotation );

    mSvgAnnotationTool = new QgsMapToolSvgAnnotation(pMapCanvas);
    mSvgAnnotationTool->setAction(mActionSvgAnnotation);

//    mFeatureAction = new QgsMapToolFeatureAction( pMapCanvas );
//    mFeatureAction->setAction( mActionFeatureAction );

      mSelectFeatures = new QgsMapToolSelectFeatures( pMapCanvas );
      mSelectFeatures->setAction( mActionSelectFeatures );
      mSelectPolygon = new QgsMapToolSelectPolygon( pMapCanvas );
    //  mSelectPolygon->setAction( mActionSelectPolygon );
      mSelectFreehand = new QgsMapToolSelectFreehand( pMapCanvas );
     // mSelectFreehand->setAction( mActionSelectFreehand );
      mSelectRadius = new QgsMapToolSelectRadius( pMapCanvas );
      mSelectRadius->setAction( mActionSelectRadius );




	//创建图层管理器
    pMapLayer=QgsMapLayerRegistry::instance();
	//添加图层

    LayerPath=QString::fromLocal8Bit(DEMODATA_DIR) + QString( "/") + QString::fromLocal8Bit( POINT_DATA) + QString(".shp"); //point data
    //设置图层
    QgsVectorLayer *pLayer1=new QgsVectorLayer(LayerPath,QString::fromLocal8Bit(POINT_DATA),"ogr");
	if(!pLayer1->isValid()){
	   QMessageBox::about(this,"load layer","Layer is not valid");
	   return;
    }
	//pLayer1->setSubLayerVisibility(QString::fromLocal8Bit("居民地"),false);
	//设置点标号
    //QgsSymbol *pSym1=new QgsSymbol(QGis::Point);  //fancy del 4 old version

    QgsStringMap properties;
      properties.insert( "color", "255,0,0,0" );
      properties.insert( "width", "0.5" );



    QgsMarkerSymbolV2 *pSyml0 = QgsMarkerSymbolV2::createSimple(properties);
    pSyml0->setColor(QColor(0,255,0));
    QgsSingleSymbolRendererV2 *mypRenderer1 = new QgsSingleSymbolRendererV2(pSyml0); //fancy add
    pLayer1->setRendererV2(mypRenderer1);
	//设置图层显示比例范围
    //pLayer1->toggleScaleBasedVisibility(true);
    //pLayer1->setMaximumScale(2000000.0);
    //pLayer1->setMinimumScale(0.0);

    //将图层添加到图层管理器
    pMapLayer->addMapLayer(pLayer1,true);
    //添加图层到图层集中
    myLayerSet.append(QgsMapCanvasLayer(pLayer1));
	checkBox->setChecked(true);


	//添加图层
    LayerPath=QString::fromLocal8Bit(DEMODATA_DIR) + QString("/") +QString::fromLocal8Bit( POLYLINE_DATA) + QString(".shp");
	//设置图层
    QgsVectorLayer *pLayer2=new QgsVectorLayer(LayerPath,QString::fromLocal8Bit(POLYLINE_DATA),"ogr");
	if(!pLayer2->isValid()){
		QMessageBox::about(this,"load layer","Layer is not valid");
		return;
	}

	//设置线标号
    QgsSymbolV2 *pSym2= QgsLineSymbolV2::defaultSymbol(QGis::Line);
    pSym2->setColor(QColor(0,0,255));
   // pSym2->setWidth(1.2);
	//设置图层绘制器
    QgsSingleSymbolRendererV2 *mypRenderer2 = new QgsSingleSymbolRendererV2(pSym2);
//	mypRenderer2->addSymbol(pSym2); fancy del
    mypRenderer2->setSymbol(pSym2); //fancy add
    pLayer2->setRendererV2(mypRenderer2);

	//将图层添加到图层管理器
	pMapLayer->addMapLayer(pLayer2,true);
	//添加图层到图层集中
	myLayerSet.append(QgsMapCanvasLayer(pLayer2));
	checkBox_2->setChecked(true);


	//添加图层
    LayerPath=QString::fromLocal8Bit(DEMODATA_DIR) + QString("/") + QString::fromLocal8Bit(POLYGON_DATA) + QString(".shp");
    //设置图层
    QgsVectorLayer *pLayer3=new QgsVectorLayer(LayerPath,QString::fromLocal8Bit(POLYGON_DATA),"ogr");
    if(!pLayer3->isValid()){
	   QMessageBox::about(this,"load layer","Layer is not valid");
	   return;
    }
    //设置面状标号
  //  QgsSymbolV2 *pSym3=new QgsSymbolV2(QGis::Polygon);

    QgsStringMap properties2;
      properties2.insert( "color", "0,0,124,0" );
      //properties2.insert( "width", "1.2" );
      //properties2.insert( "capstyle", "square" );

    QgsFillSymbolV2 *pSym3 = QgsFillSymbolV2::createSimple(properties2);
    //pSym3->setFillColor(QColor(196,230,197));
    //pSym3->setFillStyle(Qt::SolidPattern);
    //pSym3->setColor(QColor(255,255,0));
   // pSym3->setLineWidth(0.8);
	//设置图层绘制器
    QgsSingleSymbolRendererV2 *mypRenderer3 = new QgsSingleSymbolRendererV2(pSym3);
    //mypRenderer3->addSymbol(pSym3); fancy del
   mypRenderer3->setSymbol(pSym3);  //fancy add
   pLayer3->setRendererV2(mypRenderer3);
    //pLayer3->setRenderer(mypRenderer3);
    //将图层添加到图层管理器
    pMapLayer->addMapLayer(pLayer3,true);
    //添加图层到图层集中
    myLayerSet.append(QgsMapCanvasLayer(pLayer3));
	checkBox_3->setChecked(true);
    //设置绘制图层集
    pMapCanvas->setLayerSet(myLayerSet);
	//绘制图层
	pMapCanvas->zoomToFullExtent();
    pMapCanvas->refresh();
	//将图层添加到图层列表中
    listWidget->addItem(QString::fromLocal8Bit(POINT_DATA));
    listWidget->addItem(QString::fromLocal8Bit(POLYLINE_DATA));
    listWidget->addItem(QString::fromLocal8Bit(POLYGON_DATA));
	pSelectLayer=NULL;
	FieldName=tr("");
}

examp2::~examp2()
{
   delete pMapCanvas;
   //delete mpRubberBand;
}
//放大图层
void examp2::zoomInMode()
{
	pMapCanvas->setMapTool(pZoomInTool);
}
//缩小图层
void examp2::zoomOutMode()
{
    pMapCanvas->setMapTool(pZoomOutTool);
}
//移动图层
void examp2::panMode()
{
   pMapCanvas->setMapTool(pPanTool);
}
//全图显示
void examp2::FullShow()
{
	pMapCanvas->zoomToFullExtent();
}

void examp2::measrueDist()
{
    pMapCanvas->setMapTool(mMeasureDist);
}

void examp2::measrueArea()
{
    pMapCanvas->setMapTool(mMeasureArea);
}

void examp2::textAnno()
{
    pMapCanvas->setMapTool(mTextAnnotationTool);
}
void examp2::svgAnno()
{
    pMapCanvas->setMapTool(mSvgAnnotationTool);
}

void examp2::selectByExpression()
{

}
void examp2::selectFeatures()
{
    if(pMapCanvas->currentLayer() == NULL)
    {
         QMessageBox::critical(this,tr("no layer selected!"),"please choose a layer!");
         return;
    }
  pMapCanvas->setMapTool( mSelectFeatures );
}

void examp2::selectByPolygon()
{
    if(pMapCanvas->currentLayer() == NULL)
    {
         QMessageBox::critical(this,tr("no layer selected!"),"please choose a layer!");
         return;
    }
  pMapCanvas->setMapTool( mSelectPolygon );
}

void examp2::selectByFreehand()
{
    if(pMapCanvas->currentLayer() == NULL)
    {
         QMessageBox::critical(this,tr("no layer selected!"),"please choose a layer!");
         return;
    }
  pMapCanvas->setMapTool( mSelectFreehand );
}

void examp2::selectByRadius()
{
    if(pMapCanvas->currentLayer() == NULL)
    {
         QMessageBox::critical(this,tr("no layer selected!"),"please choose a layer!");
         return;
    }
  pMapCanvas->setMapTool( mSelectRadius );
}

//隐藏/显示县政府驻地图层
void examp2::EnableLayer1(bool flag)
{ 
	bool bFlag;
	QgsMapLayer *pLayer=NULL;
    QString str=QString::fromLocal8Bit(POINT_DATA);
	bFlag=QueryLayer(str,flag);
	if(bFlag){
		pMapCanvas->setLayerSet(myLayerSet);
	}
	/*else{
		QMessageBox::warning(this,QString::fromLocal8Bit("警告")，QString::fromLocal8Bit("图层没有加载"));
	}*/
}
//隐藏/显示河流图层
void examp2::EnableLayer2(bool flag)
{ 
	bool bFlag;
    QString str=QString::fromLocal8Bit(POLYLINE_DATA);
	bFlag=QueryLayer(str,flag);
	if(bFlag){
	   pMapCanvas->setLayerSet(myLayerSet);
	}
	pMapCanvas->refresh();
}
//查询需要隐藏或显示的图层，设置其可见性
bool examp2::QueryLayer(QString strLayer,bool flag)
{
    QgsMapLayer *pLayer=NULL; 
	QString str1;
    //查询图层
	int nLayCount=myLayerSet.count();
	int i;
	for(i=0;i<nLayCount;i++){
		QgsMapCanvasLayer pCanvasLayer=myLayerSet.at(i);
		pLayer=pCanvasLayer.layer();
		//得到图层名称
		str1=pLayer->name();
		if(str1==strLayer){
			//设置是否可见
			pCanvasLayer.setVisible(flag);
            myLayerSet.replace(i,pCanvasLayer);
			return true;
		}
	}
    return false;
}
//注记县政府驻地图层
void examp2::EnableLabel1(bool flag)
{
    QString str=QString::fromLocal8Bit(POINT_DATA),str1;
	int i,nLayerCount;
	QgsVectorLayer *pLayer=NULL,*pLayer1=NULL; 
	//查询居民地图层
    nLayerCount=myLayerSet.count();
    for(i=0;i<nLayerCount;i++){
		QgsMapCanvasLayer pCanvasLayer=myLayerSet.at(i);
		pLayer=(QgsVectorLayer *)pCanvasLayer.layer();
		//得到图层名称
		str1=pLayer->name();
		if(str1==str){
			pLayer1=pLayer;
			break;
		}
	}
	//如果注记的图层不存在
	if(!pLayer1){
		return;
	}
    //定义注记对象
	QgsLabel *pLabel;
    //得到该层相关联的注记对象
	pLabel=pLayer1->label();
	//定义注记属性对象
	QgsLabelAttributes *pLabelAttributes;
    //得到和注记对象相关联的属性对象
	pLabelAttributes=pLabel->labelAttributes();
	//设置注记的属性
	//设置注记的字体颜色
	pLabelAttributes->setColor(Qt::black);
    //设置注记的背景颜色
	pLabelAttributes->setBufferEnabled(true);
    pLabelAttributes->setBufferColor(Qt::yellow);
	int myType = QgsLabelAttributes::PointUnits;
    

	//设置注记的字段
	//QString str3="rname";
	pLabel->setLabelField(QgsLabel::Text,2);
	//pLabel->setLabelFieldName(0,str3);
	pLabelAttributes->setBufferSize(1,myType);
	
    pLayer1->enableLabels(flag);
    //QgsMapCanvasLayer pCanvasLayer1(pLayer1);
    //myLayerSet.replace(i,pCanvasLayer1);
	pMapCanvas->refresh();
	
}
//显示坐标
void examp2::MouseXY(const QgsPoint &p)
{
	Ui_examp2Class::statusBar->showMessage(p.toString());
	//QgsPoint myPoint1 = pMapCanvas->getCoordinateTransform()->toMapCoordinates(10, 10);
	//mpRubberBand->addPoint(p);
	//QMessageBox::about(this,tr("key"),tr("key"));
}
//从图层列表框中选择图层
void examp2::SelectLayer_0(QListWidgetItem *item)
{
	QString str,str1;
	QgsVectorLayer *pLayer=NULL,*pLayer1=NULL; 
	//条目没有被选择
	if(!item->isSelected()){
	   return; 
	}
	//得到图层名称
	str=item->text();
    //查询图层
    int i,nLayCount=pMapCanvas->layerCount();
	for(i=0;i<nLayCount;i++){
		pLayer=(QgsVectorLayer *)pMapCanvas->layer(i);
		//得到图层名称
		str1=pLayer->name();
		if(str1==str){
			pSelectLayer=pLayer;
			break;
		}
	}
	if(!pSelectLayer)return;

    pMapCanvas->setCurrentLayer(pSelectLayer); //fancy add
	pSelectTool->SetSelectLayer(pSelectLayer);
    //显示图层字段列表
	listWidget_2->clear();
    //QgsFieldMap myFields=pSelectLayer->dataProvider()->fields(); //fancy del
    QgsFields myFields=pSelectLayer->dataProvider()->fields();
    for(i=0;i<myFields.size();i++){
       listWidget_2->addItem(QString(myFields[i].name()).toLocal8Bit());
    }
}

//从图层列表框中选择图层
void examp2::SelectLayer(QListWidgetItem *item)
{
    QString str,str1;
    QgsVectorLayer *pLayer=NULL,*pLayer1=NULL;
    //条目没有被选择
    if(!item->isSelected()){
       return;
    }
    //得到图层名称
    str=item->text();
    //查询图层
    int i,nLayCount=pMapCanvas->layerCount();
    for(i=0;i<nLayCount;i++){
        pLayer=(QgsVectorLayer *)pMapCanvas->layer(i);
        //得到图层名称
        str1=pLayer->name();
        if(str1==str){
            pSelectLayer=pLayer;
            break;
        }
    }
    if(!pSelectLayer)return;

    pMapCanvas->setCurrentLayer(pSelectLayer); //fancy add
    pSelectTool->SetSelectLayer(pSelectLayer);

 this->attributeTable(pSelectLayer);
// QgisApp::instance()->layerProperties();
}

void examp2::attributeTable(QgsVectorLayer *pLayer)
{
 // QgsVectorLayer *myLayer = qobject_cast<QgsVectorLayer *>( activeLayer() );
  if ( !pLayer )
  {
    return;
  }

  QgsAttributeTableDialog *mDialog = new QgsAttributeTableDialog( pLayer );
  mDialog->show();
  // the dialog will be deleted by itself on close
}

//选择字段
void examp2::SelectField(QListWidgetItem *item)
{
    int index=-1;
	QString value,str1;
	QgsFeature feat;
	QgsField mField;
	//条目没有被选择
    if(!item->isSelected()){
	   return; 
    }
	//得到选择字段名称
	FieldName=item->text();
	////得到图层字段的存储容器
	//QgsFieldMap myFields=pSelectLayer->dataProvider()->fields();
 //   //查询选择字段在存储容器中的序号
	//QgsFieldMap::iterator it;
	//for(it=myFields.begin();it!=myFields.end();++it){
	//     mField=it.value();
	//	 str1=mField.name();
	//	 if(str1==FieldName){
	//		 index=it.key();
	//		 break;
	//	 }
	//}
	//得到字段的序号
	index=pSelectLayer->fieldNameIndex(FieldName);
	if(index==-1){
		return;
	}
	
	int in;
	QgsVectorDataProvider* mypProvider=pSelectLayer->dataProvider();
    //QgsAttributeList np=mypProvider->attributeIndexes();
	//选择所有的图元

    QgsFeatureIterator it = mypProvider->getFeatures(QgsFeatureRequest()); //QgsFeatureRequest::AllAttributes

	listWidget_3->clear();
    //while(mypProvider->nextFeature(feat)){  //fancy del
    while(it.nextFeature(feat)){

      // const QgsAttributeMap& attributes=feat.attributeMap();
       QgsAttributes  attributes = feat.attributes();
	   //得到指定序号字段的属性值
       value=attributes[index].toString();
	   if(value=="")continue;
       listWidget_3->addItem(value.toLocal8Bit());
	}
}
//选择特征
void examp2::SelectFeature(QListWidgetItem *item)
{
   QString FeatureName,SqlStr; 
   QgsFeatureIds mFeatureIds;
   //得到选择特征的属性名称
   FeatureName=item->text();
   SqlStr=FieldName+tr(" ")+tr("like")+tr(" ")+tr("'")+tr("%")+FeatureName+tr("%")+tr("'");
   //解析查询字符串建立解析树
/*   QgsSearchString search;
   if(!search.setString(SqlStr)){
      QMessageBox::critical(this,tr("Search string parsing error"),search.parserErrorMsg());
      return;
   }
   QgsSearchTreeNode* searchTree=search.tree();
   if(searchTree==NULL){
       QMessageBox::information(this,tr("Search results"),tr( "You've supplied an empty search string."));
       return;
   }
   bool fetchGeom=searchTree->needsGeometry();
   pSelectLayer->select(pSelectLayer->pendingAllAttributesList(),QgsRectangle(),fetchGeom);





   QgsFeature f;
   mFeatureIds.clear();
   while(pSelectLayer->nextFeature(f)){
	   //得到符合选择条件图元的id
	   if(searchTree->checkAgainst(pSelectLayer->pendingFields(),f)){
          mFeatureIds << f.id();
	   }
       if(searchTree->hasError())break;
   }
   if(searchTree->hasError()){
      QMessageBox::critical(this,tr("Error during search"),searchTree->errorMsg() );
      return;
   }
     */

 //  QgsExpression *expr = new QgsExpression(SqlStr);

  QgsFeatureRequest request;
  request.setFilterExpression(SqlStr);
   pSelectLayer->getFeatures(request);
   QgsFeatureIterator it =  pSelectLayer->getFeatures(request);
   QgsFeature feature;

   while(it.nextFeature(feature)){

       //QgsGeometry *geo = feature.geometry();

       //QgsPoint point = geo->asPoint();

       //qDebug() << point.x() << point.y() << endl;

     mFeatureIds.insert(feature.id());

   }

   //设置图元为选择状态
   pSelectLayer->setSelectedFeatures(mFeatureIds);
   pMapCanvas->zoomToSelected(pSelectLayer);
   //pSelectLayer->deleteSelectedFeatures();
   //pSelectLayer->setSubsetString(
}


//选择工具选择图元
void examp2::SelectElement()
{
	pSelectTool->SetEnable(true);
	pMapCanvas->setMapTool(pSelectTool);
	
}

void examp2::doFeatureAction()
{
  pMapCanvas->setMapTool( mFeatureAction );
}


//得到选择特征属性数据
void examp2::GetAttribute()
{
   QStringList strlist;
   int i,n;
   QgsSelectFeatureAttribute mAttribute;
   mAttribute.SetSelectLayer(pSelectLayer);
   strlist=mAttribute.GetSelectAttribute(tr("NAME"));
   if(!strlist.isEmpty()){
	   n=strlist.size();
	   for(i=0;i<n;i++){
	     QMessageBox::about(this,tr("ok"),strlist.at(i));
	   }
   }
}

//删除当前选择的图元
void examp2::DeleteElement()
{
   QgsDeSelectFeature pDeFeature;
   pDeFeature.SetSelectLayer(pSelectLayer);
   pDeFeature.RemoveSelectFeature(); 
   pSelectTool->SetEnable(false);
   pMapCanvas->setMapTool(pSelectTool);
}

//绘制折线
void examp2::DrawPolyLine()
{
	pMapCanvas->setMapTool(pDrawLineTool);
}

void examp2::DoOpenFly()
{
    FlyControlSetting *myPluginGui = new FlyControlSetting(this);
    //myPluginGui->SetMapCanvas( mQGisIface->mapCanvas());
    myPluginGui->show();
}


void examp2::deselectAll()
{
  // Turn off rendering to improve speed.
  bool renderFlagState = pMapCanvas->renderFlag();
  if ( renderFlagState )
    pMapCanvas->setRenderFlag( false );

  QMap<QString, QgsMapLayer*> layers = QgsMapLayerRegistry::instance()->mapLayers();
  for ( QMap<QString, QgsMapLayer*>::iterator it = layers.begin(); it != layers.end(); ++it )
  {
    QgsVectorLayer *vl = qobject_cast<QgsVectorLayer *>( it.value() );
    if ( !vl )
      continue;

    vl->removeSelection();
  }

  // Turn on rendering (if it was on previously)
  if ( renderFlagState )
    pMapCanvas->setRenderFlag( true );
}

