#include "mymainwindow.h"
#include "ui_mymainwindow.h"
#include <QMdiSubWindow>
#include "qsettings.h"
#include "qfileinfo.h"
#include "qmessagebox.h"
#include "qgsmaplayerregistry.h"
#include "qgsproviderregistry.h"
#include "qgsproject.h"
#include "qgslayertreemodel.h"
#include "qtoolbutton.h"
#include "qgsmessagebaritem.h"
#include "qgraphicsscene.h"
#include "qfiledialog.h"
#include "qgslogger.h"
#include "qgsannotationitem.h"
#include "qdockwidget.h"
#include "qgsvisibilitypresets.h"
#include "qtoolbar.h"
#include "qgslayertree.h"
#include "qgslayertreeregistrybridge.h"

#include "qgsmaptooltextannotation.h"
#include "qgsmaptoolsvgannotation.h"
#include "qgsmaptoolfeatureaction.h"
#include "qgsmaptoolselect.h"
#include "qgsmaptoolselectfreehand.h"
#include "qgsmaptoolselectradius.h"
#include "qgsmaptoolselectrectangle.h"
#include "qgsmaptoolselectpolygon.h"
#include "qgsmaptoolpan.h"
#include "qgsmaptoolzoom.h"
#include "qgsmeasuretool.h"
#include "qgssinglesymbolrendererv2.h"
#include "qgsrasterlayer.h"
#include "qgsmultibandcolorrenderer.h"


#include <osg/Notify>
#include <osg/Version>
#include <osgEarth/ImageUtils>
#include <osgEarth/MapNode>
#include <osgEarthAnnotation/AnnotationData>
#include <osgEarthAnnotation/AnnotationNode>
#include <osgEarthAnnotation/PlaceNode>
#include <osgEarthAnnotation/ScaleDecoration>
#include <osgEarthAnnotation/TrackNode>
#include <osgEarthQt/ViewerWidget>
#include <osgViewer/ViewerBase>
#include <osgViewer/CompositeViewer>
#include <osgEarthQt/LOSControlWidget>
#include <osgEarthQt/TerrainProfileWidget>
#include <osgEarthUtil/AutoClipPlaneHandler>
#include <osgEarthUtil/EarthManipulator>
#include <osgEarthUtil/Sky>
#include <osgEarthUtil/Ocean>


#ifdef Q_WS_X11
#include <X11/Xlib.h>
#endif

using namespace osgEarth::Util;

#define TRACK_ICON_URL    "../data/m2525_air.png"
#define TRACK_ICON_SIZE   24
#define TRACK_FIELD_NAME  "name"

static osg::ref_ptr<osg::Group> s_annoGroup;
static osgEarth::Util::SkyNode* s_sky=0L;
static osgEarth::Util::OceanNode* s_ocean=0L;

const char* PLUGINS_DIR1 = "/usr/local/lib/qgis/plugins";

myMainWindow* myMainWindow::smInstance = 0;
myMainWindow::myMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::myMainWindow)
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

    ui->setupUi(this);

    //注册QGIS插件
    QString myPluginsDir(PLUGINS_DIR1);
    QgsProviderRegistry::instance(myPluginsDir);

    //创建地图显示画板
    mMapCanvas=new QgsMapCanvas(this, "二维窗口");
    mMapCanvas->enableAntiAliasing(true);
    mMapCanvas->setCanvasColor(QColor(255, 255, 255));
    mMapCanvas->freeze(false);
    mMapCanvas->setVisible(true);
    mMapCanvas->setFocus();

    mMdiArea = new QMdiArea;
    mMdiArea->setViewMode(QMdiArea::TabbedView);
    mMdiArea->setTabPosition(QTabWidget::South);
    mMdiArea->setTabsClosable(false);
    mMdiArea->setTabShape(QTabWidget::Triangular);

    QMdiSubWindow *p2DWin = mMdiArea->addSubWindow(mMapCanvas);
    p2DWin->setWindowTitle(tr("二维窗口"));
    p2DWin->setWindowState(Qt::WindowMaximized);

    //设为中心Widget
    setCentralWidget(mMdiArea);

    connect(mMapCanvas,SIGNAL(xyCoordinates(const QgsPoint &)),this,SLOT(MouseXY(const QgsPoint &)));
    mMapCanvas->freeze();
    mMapCanvas->setWindowState(Qt::WindowMaximized);

    mLayerTreeView = new QgsLayerTreeView( this );
    mLayerTreeView->setObjectName( "theLayerTreeView" ); //
    initLayerTreeView();
    mInfoBar = new QgsMessageBar( centralWidget() );

    //创建地图工具
    createMapTools();
}

myMainWindow::~myMainWindow()
{
    delete pPanTool;
    delete pZoomInTool;
    delete pZoomOutTool;
    delete mSelectFeatures;
    delete ui;

}

void myMainWindow::createMapTools()
{
    //地图操作工具
    pPanTool = new QgsMapToolPan(mMapCanvas);
    pZoomInTool=new QgsMapToolZoom(mMapCanvas, FALSE); // false = in
    pZoomOutTool=new QgsMapToolZoom(mMapCanvas, TRUE ); //true = out
    mSelectFeature = new QgsMapToolSelect(mMapCanvas);
    mSelectFeatures = new QgsMapToolSelectFeatures( mMapCanvas );
    // mSelectFeatures->setAction(actionSelectFeature  );


    pDrawLineTool=new QgsMapToolDrawLine(mMapCanvas);
    mMeasureDist = new QgsMeasureTool(mMapCanvas,false); //fancy 测量工具来自 APP
    mMeasureArea = new QgsMeasureTool( mMapCanvas, true /* area */ );
    mTextAnnotationTool = new QgsMapToolTextAnnotation( mMapCanvas );
    mSvgAnnotationTool = new QgsMapToolSvgAnnotation(mMapCanvas);
}

void myMainWindow::on_actionOpenPro_triggered()
{
    // Retrieve last used project dir from persistent settings
    QSettings settings;
    QString lastUsedDir = settings.value( "/UI/lastProjectDir", "." ).toString();
    QString fullPath = QFileDialog::getOpenFileName( this,
                                                     tr( "Choose a QGIS project file to open" ),
                                                     lastUsedDir,
                                                     tr( "QGIS files" ) + " (*.qgs *.QGS)" );
    if ( fullPath.isNull() )
    {
        return;
    }

    // Fix by Tim - getting the dirPath from the dialog
    // directly truncates the last node in the dir path.
    // This is a workaround for that
    QFileInfo myFI( fullPath );
    QString myPath = myFI.path();
    // Persist last used project dir
    settings.setValue( "/UI/lastProjectDir", myPath );

    // open the selected project
    addProject( fullPath );

}

/**
  adds a saved project to qgis, usually called on startup by specifying a
  project file on the command line
  */
bool myMainWindow::addProject( QString projectFile )
{
    QFileInfo pfi( projectFile );
    statusBar()->showMessage( tr( "Loading project: %1" ).arg( pfi.fileName() ) );
    qApp->processEvents();

    QApplication::setOverrideCursor( Qt::WaitCursor );

    // close the previous opened project if any
    closeProject();

    if ( !QgsProject::instance()->read( pfi ) )
    {
        QApplication::restoreOverrideCursor();
        statusBar()->clearMessage();

        QMessageBox::critical( this,
                               tr( "Unable to open project" ),
                               QgsProject::instance()->error() );


        mMapCanvas->freeze( false );
        mMapCanvas->refresh();
        return false;
    }

    //mProjectLastModified = pfi.lastModified();
    // setTitleBarText_( *this );
    int  myRedInt = QgsProject::instance()->readNumEntry( "Gui", "/CanvasColorRedPart", 255 );
    int  myGreenInt = QgsProject::instance()->readNumEntry( "Gui", "/CanvasColorGreenPart", 255 );
    int  myBlueInt = QgsProject::instance()->readNumEntry( "Gui", "/CanvasColorBluePart", 255 );
    QColor myColor = QColor( myRedInt, myGreenInt, myBlueInt );
    mMapCanvas->setCanvasColor( myColor ); //this is fill color before rendering starts
    QgsDebugMsg( "Canvas background color restored..." );

    //load project scales
    bool projectScales = QgsProject::instance()->readBoolEntry( "Scales", "/useProjectScales" );
    if ( projectScales )
    {
        //mScaleEdit->updateScales( QgsProject::instance()->readListEntry( "Scales", "/ScalesList" ) );
    }

    mMapCanvas->updateScale();
    QgsDebugMsg( "Scale restored..." );

    setFilterLegendByMapEnabled( QgsProject::instance()->readBoolEntry( "Legend", "filterByMap" ) );

    QSettings settings;

    // does the project have any macros?

    emit projectRead(); // let plug-ins know that we've read in a new
    // project so that they can check any project
    // specific plug-in state

    // add this to the list of recently used project files
    // saveRecentProjectPath( projectFile, settings );

    QApplication::restoreOverrideCursor();

    mMapCanvas->freeze( false );
    mMapCanvas->refresh();

    statusBar()->showMessage( tr( "Project loaded" ), 3000 );
    return true;
}


void myMainWindow::closeProject()
{
    // unload the project macros before changing anything
    //if ( mTrustedMacros )
    //{
    //  QgsPythonRunner::run( "qgis.utils.unloadProjectMacros();" );
    // }

    // remove any message widgets from the message bar
    mInfoBar->clearWidgets();

    // mTrustedMacros = false;

    setFilterLegendByMapEnabled( false );

    //deletePrintComposers();
    removeAnnotationItems();
    // clear out any stuff from project
    removeAllLayers();

    mMapCanvas->freeze( true );
    QList<QgsMapCanvasLayer> emptyList;
    mMapCanvas->setLayerSet( emptyList );
    mMapCanvas->clearCache();
    mMapCanvas->refresh();
}

//主程序退出
void myMainWindow::on_actionExit_triggered()
{
    this->close();
}


void myMainWindow::removeAllLayers()
{
    QgsMapLayerRegistry::instance()->removeAllMapLayers();
    mLayerTreeView->layerTreeModel()->rootGroup()->removeAllChildren();
    QgsProject::instance()->dirty(true);
}

QgsLayerTreeView* myMainWindow::layerTreeView()
{
    Q_ASSERT( mLayerTreeView );
    return mLayerTreeView;
}


void myMainWindow::setFilterLegendByMapEnabled( bool enabled )
{
    QgsLayerTreeModel* model = layerTreeView()->layerTreeModel();
    bool wasEnabled = model->legendFilterByMap();
    if ( wasEnabled == enabled )
        return; // no change

    mActionFilterLegend->setChecked( enabled );

    if ( enabled )
    {
        connect( mMapCanvas, SIGNAL( mapCanvasRefreshed() ), this, SLOT( updateFilterLegendByMap() ) );
        model->setLegendFilterByMap( &mMapCanvas->mapSettings() );
    }
    else
    {
        disconnect( mMapCanvas, SIGNAL( mapCanvasRefreshed() ), this, SLOT( updateFilterLegendByMap() ) );
        model->setLegendFilterByMap( 0 );
    }
}

void myMainWindow::removeAnnotationItems()
{
    if ( !mMapCanvas )
    {
        return;
    }
    QGraphicsScene* scene = mMapCanvas->scene();
    if ( !scene )
    {
        return;
    }
    QList<QgsAnnotationItem*> itemList = annotationItems();
    QList<QgsAnnotationItem*>::iterator itemIt = itemList.begin();
    for ( ; itemIt != itemList.end(); ++itemIt )
    {
        if ( *itemIt )
        {
            scene->removeItem( *itemIt );
            delete *itemIt;
        }
    }
}

QList<QgsAnnotationItem*> myMainWindow::annotationItems()
{
    QList<QgsAnnotationItem*> itemList;

    if ( !mMapCanvas )
    {
        return itemList;
    }

    if ( mMapCanvas )
    {
        QList<QGraphicsItem*> graphicsItems = mMapCanvas->items();
        QList<QGraphicsItem*>::iterator gIt = graphicsItems.begin();
        for ( ; gIt != graphicsItems.end(); ++gIt )
        {
            QgsAnnotationItem* currentItem = dynamic_cast<QgsAnnotationItem*>( *gIt );
            if ( currentItem )
            {
                itemList.push_back( currentItem );
            }
        }
    }
    return itemList;
}

void myMainWindow::initLayerTreeView()
{
    mLayerTreeView->setWhatsThis( tr( "Map legend that displays all the layers currently on the map canvas. Click on the check box to turn a layer on or off. Double click on a layer in the legend to customize its appearance and set other properties." ) );
    mLayerTreeDock = new QDockWidget( tr("图层"), this );
    mLayerTreeDock->setObjectName( "Layers" );
    mLayerTreeDock->setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );

    QgsLayerTreeModel* model = new QgsLayerTreeModel( QgsProject::instance()->layerTreeRoot(), this );
#ifdef ENABLE_MODELTEST
    new ModelTest( model, this );
#endif
    model->setFlag( QgsLayerTreeModel::AllowNodeReorder );
    model->setFlag( QgsLayerTreeModel::AllowNodeRename );
    model->setFlag( QgsLayerTreeModel::AllowNodeChangeVisibility );
    model->setFlag( QgsLayerTreeModel::ShowLegendAsTree );
    model->setFlag( QgsLayerTreeModel::AllowSymbologyChangeState);
    model->setAutoCollapseLegendNodes( 10 );

    mLayerTreeView->setModel( model );
    //fancy del  //mLayerTreeView->setMenuProvider( new QgsAppLayerTreeViewMenuProvider( mLayerTreeView, mMapCanvas ) );

    setupLayerTreeViewFromSettings();

    connect( mLayerTreeView, SIGNAL( doubleClicked( QModelIndex ) ), this, SLOT( layerTreeViewDoubleClicked( QModelIndex ) ) );
    connect( mLayerTreeView, SIGNAL( currentLayerChanged( QgsMapLayer* ) ), this, SLOT( activeLayerChanged( QgsMapLayer* ) ) );
    connect( mLayerTreeView->selectionModel(), SIGNAL( currentChanged( QModelIndex, QModelIndex ) ), this, SLOT( updateNewLayerInsertionPoint() ) );
    connect( QgsProject::instance()->layerTreeRegistryBridge(), SIGNAL( addedLayersToLayerTree( QList<QgsMapLayer*> ) ),
             this, SLOT( autoSelectAddedLayer( QList<QgsMapLayer*> ) ) );

    // add group action
    QAction* actionAddGroup = new QAction( tr( "添加组图层" ), this );
    actionAddGroup->setIcon(QIcon(":/Resources/mActionAddGroupLayer.png" ) );
    actionAddGroup->setToolTip( tr( "添加组图层" ));
    connect( actionAddGroup, SIGNAL( triggered( bool)), this, SLOT(on_actionAddGroupLayer_triggered()));

    // visibility groups tool button
    QToolButton* btnVisibilityPresets = new QToolButton;
    btnVisibilityPresets->setAutoRaise( true );
    btnVisibilityPresets->setToolTip( tr( "Manage Layer Visibility" ) );
    btnVisibilityPresets->setIcon( QgsApplication::getThemeIcon( "/mActionShowAllLayers.svg" ) );
    btnVisibilityPresets->setPopupMode( QToolButton::InstantPopup );
    // btnVisibilityPresets->setMenu( QgsVisibilityPresets::instance()->menu() );

    // filter legend action
    mActionFilterLegend = new QAction( tr( "Filter Legend By Map Content" ), this );
    mActionFilterLegend->setCheckable( true );
    mActionFilterLegend->setToolTip( tr( "Filter Legend By Map Content" ) );
    mActionFilterLegend->setIcon( QgsApplication::getThemeIcon( "/mActionFilter2.svg" ) );
    connect( mActionFilterLegend, SIGNAL( triggered( bool ) ), this, SLOT( toggleFilterLegendByMap() ) );

    // expand / collapse tool buttons
    QAction* actionExpandAll = new QAction( tr( "全部展开" ), this );
    actionExpandAll->setIcon( QIcon(":/Resources/mActionExpand.png" ) );
    actionExpandAll->setToolTip( tr( "全部展开" ) );
    connect( actionExpandAll, SIGNAL( triggered( bool ) ), mLayerTreeView, SLOT( expandAll() ) );
    QAction* actionCollapseAll = new QAction( tr( "全部收起" ), this );
    actionCollapseAll->setIcon( QIcon( ":/Resources/mActionCollapse.png" ) );
    actionCollapseAll->setToolTip( tr( "全部收起" ) );
    connect( actionCollapseAll, SIGNAL( triggered( bool ) ), mLayerTreeView, SLOT( collapseAll() ) );

    QToolBar* toolbar = new QToolBar();
    toolbar->setIconSize( QSize( 16, 16 ) );
    toolbar->addAction( actionAddGroup );
    //toolbar->addWidget( btnVisibilityPresets );
    //toolbar->addAction( mActionFilterLegend );
    toolbar->addAction( actionExpandAll );
    toolbar->addAction( actionCollapseAll );
    //toolbar->addAction( actionRemoveLayer );

    QVBoxLayout* vboxLayout = new QVBoxLayout;
    vboxLayout->setMargin( 0 );
    vboxLayout->addWidget( toolbar );
    vboxLayout->addWidget( mLayerTreeView );

    QWidget* w = new QWidget;
    w->setLayout( vboxLayout );
    mLayerTreeDock->setWidget( w );
    addDockWidget( Qt::LeftDockWidgetArea, mLayerTreeDock );

    mLayerTreeCanvasBridge = new QgsLayerTreeMapCanvasBridge( QgsProject::instance()->layerTreeRoot(), mMapCanvas, this );
    connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument& ) ), mLayerTreeCanvasBridge, SLOT( writeProject( QDomDocument& ) ) );
    connect( QgsProject::instance(), SIGNAL( readProject( QDomDocument ) ), mLayerTreeCanvasBridge, SLOT( readProject( QDomDocument ) ) );

    bool otfTransformAutoEnable = QSettings().value( "/Projections/otfTransformAutoEnable", true ).toBool();
    mLayerTreeCanvasBridge->setAutoEnableCrsTransform( otfTransformAutoEnable );

    mMapLayerOrder = new QgsCustomLayerOrderWidget( mLayerTreeCanvasBridge, this );
    mMapLayerOrder->setObjectName( "theMapLayerOrder" );

    mMapLayerOrder->setWhatsThis( tr( "Map layer list that displays all layers in drawing order." ) );
    mLayerOrderDock = new QDockWidget( tr( "Layer order" ), this );
    mLayerOrderDock->setObjectName( "LayerOrder" );
    mLayerOrderDock->setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );

    mLayerOrderDock->setWidget( mMapLayerOrder );
    addDockWidget( Qt::LeftDockWidgetArea, mLayerOrderDock );
    mLayerOrderDock->hide();
}


void myMainWindow::updateNewLayerInsertionPoint()
{
    // defaults
    QgsLayerTreeGroup* parentGroup = mLayerTreeView->layerTreeModel()->rootGroup();
    int index = 0;
    QModelIndex current = mLayerTreeView->currentIndex();

    if ( current.isValid() )
    {
        if ( QgsLayerTreeNode* currentNode = mLayerTreeView->currentNode() )
        {
            // if the insertion point is actually a group, insert new layers into the group
            if ( QgsLayerTree::isGroup( currentNode ) )
            {
                QgsProject::instance()->layerTreeRegistryBridge()->setLayerInsertionPoint( QgsLayerTree::toGroup( currentNode ), 0 );
                return;
            }

            // otherwise just set the insertion point in front of the current node
            QgsLayerTreeNode* parentNode = currentNode->parent();
            if ( QgsLayerTree::isGroup( parentNode ) )
                parentGroup = QgsLayerTree::toGroup( parentNode );
        }

        index = current.row();
    }

    QgsProject::instance()->layerTreeRegistryBridge()->setLayerInsertionPoint( parentGroup, index );
}

void myMainWindow::setupLayerTreeViewFromSettings()
{
    QSettings s;

    QgsLayerTreeModel* model = mLayerTreeView->layerTreeModel();
    model->setFlag( QgsLayerTreeModel::ShowRasterPreviewIcon, s.value( "/qgis/createRasterLegendIcons", false ).toBool() );

    QFont fontLayer, fontGroup;
    fontLayer.setBold( s.value( "/qgis/legendLayersBold", true ).toBool() );
    fontGroup.setBold( s.value( "/qgis/legendGroupsBold", false ).toBool() );
    model->setLayerTreeNodeFont( QgsLayerTreeNode::NodeLayer, fontLayer );
    model->setLayerTreeNodeFont( QgsLayerTreeNode::NodeGroup, fontGroup );
}

void myMainWindow::activeLayerChanged( QgsMapLayer* layer )
{
    if ( NULL != mMapCanvas )
        mMapCanvas->setCurrentLayer( layer );
}

void myMainWindow::autoSelectAddedLayer( QList<QgsMapLayer*> layers )
{
    if ( layers.count() )
    {
        QgsLayerTreeLayer* nodeLayer = QgsProject::instance()->layerTreeRoot()->findLayer( layers[0]->id() );

        if ( !nodeLayer )
            return;

        QModelIndex index = mLayerTreeView->layerTreeModel()->node2index( nodeLayer );
        mLayerTreeView->setCurrentIndex( index );
    }
}

void myMainWindow::on_actionPntSel_triggered()
{
    if(mMapCanvas->currentLayer() == NULL)
    {
        QMessageBox::critical(this,tr("未选择图层!"),"请选择图层!");
        return;
    }

    mMapCanvas->setMapTool(mSelectFeature);
}

void myMainWindow::setupConnections()
{
    // connect legend signals
    connect( mLayerTreeView, SIGNAL( currentLayerChanged( QgsMapLayer * ) ),
             this, SLOT( activateDeactivateLayerRelatedActions( QgsMapLayer * ) ) );
    connect( mLayerTreeView->selectionModel(), SIGNAL( selectionChanged( QItemSelection, QItemSelection ) ),
             this, SLOT( legendLayerSelectionChanged() ) );
    connect( mLayerTreeView->layerTreeModel()->rootGroup(), SIGNAL( addedChildren( QgsLayerTreeNode*, int, int ) ),
             this, SLOT( markDirty() ) );
    connect( mLayerTreeView->layerTreeModel()->rootGroup(), SIGNAL( addedChildren( QgsLayerTreeNode*, int, int ) ),
             this, SLOT( updateNewLayerInsertionPoint() ) );
    connect( mLayerTreeView->layerTreeModel()->rootGroup(), SIGNAL( removedChildren( QgsLayerTreeNode*, int, int ) ),
             this, SLOT( markDirty() ) );
    connect( mLayerTreeView->layerTreeModel()->rootGroup(), SIGNAL( removedChildren( QgsLayerTreeNode*, int, int ) ),
             this, SLOT( updateNewLayerInsertionPoint() ) );
    connect( mLayerTreeView->layerTreeModel()->rootGroup(), SIGNAL( visibilityChanged( QgsLayerTreeNode*, Qt::CheckState ) ),
             this, SLOT( markDirty() ) );
    connect( mLayerTreeView->layerTreeModel()->rootGroup(), SIGNAL( customPropertyChanged( QgsLayerTreeNode*, QString ) ),
             this, SLOT( markDirty() ) );
    connect( mLayerTreeView->layerTreeModel()->rootGroup(), SIGNAL( customPropertyChanged( QgsLayerTreeNode*, QString ) ),
             this, SLOT( markDirty() ) );


    // connect map layer registry
    connect( QgsMapLayerRegistry::instance(), SIGNAL( layersAdded( QList<QgsMapLayer *> ) ),
             this, SLOT( layersWereAdded( QList<QgsMapLayer *> ) ) );
    connect( QgsMapLayerRegistry::instance(),
             SIGNAL( layersWillBeRemoved( QStringList ) ),
             this, SLOT( removingLayers( QStringList ) ) );

}


//打开地图工程
void myMainWindow::on_actionOpenProject_triggered()
{
    // Retrieve last used project dir from persistent settings
    QSettings settings;
    QString lastUsedDir = settings.value( "/UI/lastProjectDir", "." ).toString();
    QString qsProjectFile = QFileDialog::getOpenFileName( this,
                                                          tr( "打开工程文件" ),
                                                          lastUsedDir,
                                                          tr( "工程文件" ) + " (*.gmd *.GMD)" );

    if ( qsProjectFile.isNull()) return;

    // Fix by Tim - getting the dirPath from the dialog
    // directly truncates the last node in the dir path.
    // This is a workaround for that
    QFileInfo qfiProject( qsProjectFile );
    QString qsProjFolder = qfiProject.path();

    // Persist last used project dir
    settings.setValue( "/UI/lastProjectDir", qsProjFolder );

    // open the selected project
    addProject( qsProjectFile );
}

//
void myMainWindow::on_actionSaveProject_triggered()
{
    QgsProject *qgsProj = QgsProject::instance();
    if(qgsProj->isDirty())
        qgsProj->write(qgsProj->fileName());
}

//保存工程
void myMainWindow::on_actionSaveAsProject_triggered()
{
    QSettings settings;
    QString lastUsedDir = settings.value( "/UI/lastProjectDir", "." ).toString();

    QString qsProjectFile = QFileDialog::getSaveFileName( this,
                                                          tr( "另存工程文件为..." ),
                                                          lastUsedDir,
                                                          tr( "工程文件" ) + " (*.qgs *.QGS)" );
    QgsProject *qgsProj = QgsProject::instance();
    qgsProj->dirty(true);
    const QFileInfo qsProjInfo(qsProjectFile);
    qgsProj->write(qsProjInfo);
}


//关闭地图工程
void myMainWindow::on_actionCloseProject_triggered()
{
    closeProject();
}

void myMainWindow::toggleFilterLegendByMap()
{
    bool enabled = layerTreeView()->layerTreeModel()->legendFilterByMap();
    setFilterLegendByMapEnabled( !enabled );
}

//地图放大
void myMainWindow::on_actionZoomIn_triggered()
{
    mMapCanvas->setMapTool(pZoomInTool);
}

//地图缩小
void myMainWindow::on_actionZoomout_triggered()
{
    mMapCanvas->setMapTool(pZoomOutTool);
}

//移动地图
void myMainWindow::on_actionMove_triggered()
{
    mMapCanvas->setMapTool(pPanTool);
}

//全图显示
void myMainWindow::on_actionFullMap_triggered()
{
    mMapCanvas->zoomToFullExtent();
}

//刷新
void myMainWindow::on_actionRefresh_triggered()
{
    mMapCanvas->refresh();
}

//前一视图
void myMainWindow::on_actionPreviousView_triggered()
{
    mMapCanvas->zoomToPreviousExtent();
}

//后一视图
void myMainWindow::on_actionNextView_triggered()
{
    mMapCanvas->zoomToNextExtent();
}

//选择
void myMainWindow::on_actionSelectFeature_triggered()
{
    mMapCanvas->setMapTool(mSelectFeature);
}

//添加矢量图层
void myMainWindow::on_actionAddVectorLayer_triggered()
{
    QSettings settings;
    QString lastUsedDir = settings.value( "/UI/lastProjectDir", "." ).toString();

    QString LayerPath = QFileDialog::getOpenFileName( this,
                                                      tr( "添加矢量图层" ),
                                                      lastUsedDir,
                                                      tr( "矢量数据文件" ) + " (*.shp *.SHP *.tab *.TAB)" );

    if(LayerPath.isEmpty()) return;

    QFileInfo myFI( LayerPath );
    //设置图层，图层提供者为"ogr",图层名称为""。
    QgsVectorLayer *pLayer = new QgsVectorLayer(LayerPath, myFI.fileName(),"ogr");
    if(!pLayer->isValid()){
        QMessageBox::about(this,"加载图层","待加载图层无效.");
        return;
    }

    QgsMapLayerRegistry::instance()->addMapLayer(pLayer,true);
    QList<QgsMapCanvasLayer> myLayerSet;
    myLayerSet.append(QgsMapCanvasLayer(pLayer));
    mMapCanvas->setLayerSet(myLayerSet);
    QgsProject::instance()->dirty(true);
}

//添加栅格图层
void myMainWindow::on_actionAddRasterLayer_triggered()
{
    QSettings settings;
    QString lastUsedDir = settings.value( "/UI/lastProjectDir", "." ).toString();

    QString LayerPath = QFileDialog::getOpenFileName( this,
                                                      tr( "添加栅格图层" ),
                                                      lastUsedDir,
                                                      tr( "栅格数据文件" ) + " (*.tif *.TIF *.jpg *.JPG)" );

    if(LayerPath.isEmpty()) return;

    QFileInfo myFI( LayerPath );
    //设置图层，图层提供者为"ogr",图层名称为""。
    QgsRasterLayer *pLayer = new QgsRasterLayer(LayerPath, myFI.completeBaseName());
    if(!pLayer->isValid()){
        QMessageBox::about(this,"加载图层","待加载图层无效.");
        return;
    }

    QgsCoordinateReferenceSystem sourceCRS;
    sourceCRS.createFromId( 4326, QgsCoordinateReferenceSystem::EpsgCrsId );
    pLayer->setCrs( sourceCRS, false );

    QgsMultiBandColorRenderer* rasterRenderer = new QgsMultiBandColorRenderer( pLayer->dataProvider(), 2, 3, 4 );
    pLayer->setRenderer( rasterRenderer );

    QList<QgsMapCanvasLayer> myLayerSet;
    QgsMapLayerRegistry::instance()->addMapLayer(pLayer,TRUE);
    myLayerSet.append(QgsMapCanvasLayer(pLayer));
    mMapCanvas->setLayerSet(myLayerSet);
    QgsProject::instance()->dirty(true);

}

//添加地图服务
void myMainWindow::on_actionAddMapService_triggered()
{

}

//添加图层组
void myMainWindow::on_actionAddGroupLayer_triggered()
{
    QgsLayerTreeGroup* parentGroup = mLayerTreeView->layerTreeModel()->rootGroup();
    parentGroup->addGroup(tr("新建图层组"));
    QgsProject::instance()->dirty(true);
}

//移除图层
void myMainWindow::on_actionRemoveLayer_triggered()
{
    if(mMapCanvas->currentLayer() != NULL)
    {
        QgsMapLayerRegistry::instance()->removeMapLayer(mMapCanvas->currentLayer()->id());
        QgsProject::instance()->dirty(true);
    }
    else if(mLayerTreeView->currentNode() != NULL)
    {
        mLayerTreeView->layerTreeModel()->rootGroup()->removeChildNode(mLayerTreeView->currentNode());
        QgsProject::instance()->dirty(true);
    }

}

void myMainWindow::on_actionSelectFeatures_triggered()
{
    if(mMapCanvas->currentLayer() == NULL)
    {
        QMessageBox::critical(this,tr("未选择图层!"),"请选择图层!");
        return;
    }
    mMapCanvas->setMapTool(mSelectFeatures);
}

void myMainWindow::on_actionClear_triggered()
{
    // Turn off rendering to improve speed.
    bool renderFlagState = mMapCanvas->renderFlag();
    if ( renderFlagState )
        mMapCanvas->setRenderFlag( false );

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
        mMapCanvas->setRenderFlag( true );
}

void myMainWindow::on_actionDist_triggered()
{
    mMapCanvas->setMapTool(mMeasureDist);
}

void myMainWindow::on_actionArea_triggered()
{
    mMapCanvas->setMapTool(mMeasureArea);
}

void myMainWindow::on_actionText_triggered()
{
    mMapCanvas->setMapTool(mTextAnnotationTool);
}

void myMainWindow::on_actionSVG_triggered()
{
    mMapCanvas->setMapTool(mSvgAnnotationTool);
}

void myMainWindow::on_actionLine_triggered()
{
    statusBar()->showMessage(tr("draw a line"));
    mMapCanvas->setMapTool(pDrawLineTool);
}

void myMainWindow::on_actionZone_triggered()
{

}

void myMainWindow::on_action3DView_triggered(bool bChecked)
{
    create3DView2();
}

void myMainWindow::create3DView2()
{
    QStringList strList;
    strList.append("/home/geobeans/mapcode/qgis-depends/gwaldron-osgearth-25ce0e1/tests/mapquest_osm.earth");
    QProcess::startDetached("/usr/local/bin/osgearth_qt_simple", strList);
}

void myMainWindow::create3DView()
{
    // load the .earth file from the command line.
    osg::DisplaySettings::instance()->setMinimumNumStencilBits(8);
    std::string strEarthFile = "/home/geobeans/mapcode/qgis-depends/gwaldron-osgearth-25ce0e1/tests/mapquest_osm.earth";
    osg::ref_ptr<osg::Node> earthNode = osgDB::readNodeFile(strEarthFile);
    if (!earthNode)
        return;

    osg::Group* root = new osg::Group();
    root->addChild( earthNode );

    s_annoGroup = new osg::Group();
    root->addChild( s_annoGroup );

    //    osg::ref_ptr<osgEarth::MapNode> mapNode = osgEarth::MapNode::findMapNode(earthNode );
    //    osgEarth::QtGui::ViewVector views;
    //    osg::ref_ptr<osgViewer::ViewerBase> viewer;
    //    mViewerWidget = NULL;
    //    osgViewer::Viewer* v = new osgViewer::Viewer();
    //    v->setSceneData(root);
    //    //v->setThreadingModel(osgViewer::Viewer::DrawThreadPerContext);
    //    v->setCameraManipulator(new osgEarth::Util::EarthManipulator());
    //    mViewerWidget = new osgEarth::QtGui::ViewerWidget(v);
    //    mViewerWidget->getViews( views );

    //    for(osgEarth::QtGui::ViewVector::iterator i = views.begin(); i != views.end(); ++i )
    //    {
    //        i->get()->getCamera()->addCullCallback(new osgEarth::Util::AutoClipPlaneCullCallback(mapNode));
    //    }

    //    m3DWin = mMdiArea->addSubWindow(mViewerWidget);
    //    m3DWin->resize(500, 600);
    //    m3DWin->setWindowTitle(tr("三维窗口"));
    //    m3DWin->show();
    //    mViewerWidget->getViews( views );
    //    if (mapNode.valid())
    //    {
    //        const Config& externals = mapNode->externalConfig();

    //        if (mapNode->getMap()->isGeocentric())
    //        {
    //            // Sky model.
    //            Config skyConf = externals.child("sky");

    //            double hours = skyConf.value("hours", 12.0);
    //            s_sky = osgEarth::Util::SkyNode::create(mapNode);
    //            s_sky->setDateTime( DateTime(2011, 3, 6, hours) );
    //            for(osgEarth::QtGui::ViewVector::iterator i = views.begin(); i != views.end(); ++i )
    //                s_sky->attach( *i, 0 );
    //            root->addChild(s_sky);

    //            // Ocean surface.
    //            if (externals.hasChild("ocean"))
    //            {
    //                s_ocean = osgEarth::Util::OceanNode::create(
    //                            osgEarth::Util::OceanOptions(externals.child("ocean")),
    //                            mapNode.get());

    //                if (s_ocean)
    //                    root->addChild(s_ocean);
    //            }
    //        }
    //    }

    //    viewer = mViewerWidget->getViewer();

    //    TrackSimVector trackSims;
    //    if ( trackData )
    //    {
    //        // create demo tracks
    //        osg::ref_ptr<osg::Image> srcImage = osgDB::readImageFile(TRACK_ICON_URL);
    //        osg::ref_ptr<osg::Image> image;
    //        ImageUtils::resizeImage(srcImage.get(), TRACK_ICON_SIZE, TRACK_ICON_SIZE, image);

    //        TrackNodeFieldSchema schema;
    //        createTrackSchema(schema);
    //        dataManager->addAnnotation(createTrack(schema, image, "Plane 1", mapNode.get(), osg::Vec3d(-121.463, 46.3548, 1500.71), 10000, 24, trackSims), s_annoGroup);
    //        dataManager->addAnnotation(createTrack(schema, image, "Plane 2", mapNode.get(), osg::Vec3d(-121.656, 46.0935, 4133.06), 10000, 8, trackSims), s_annoGroup);
    //        dataManager->addAnnotation(createTrack(schema, image, "Plane 3", mapNode.get(), osg::Vec3d(-121.321, 46.2589, 1390.09), 10000, 12, trackSims), s_annoGroup);

    //        viewer->addUpdateOperation(new TrackSimUpdate(trackSims));
    //    }
    //    // activate "on demand" rendering if requested:
    //    //viewer->setRunFrameScheme( osgViewer::ViewerBase::ON_DEMAND );
    //    if(viewer.valid())
    //        viewer->setThreadingModel(osgViewer::ViewerBase::SingleThreaded);

    osgViewer::Viewer viewer;
    viewer.setRunFrameScheme( viewer.ON_DEMAND );
    viewer.setCameraManipulator( new EarthManipulator() );
    viewer.setThreadingModel(osgViewer::ViewerBase::SingleThreaded);
    viewer.setSceneData( earthNode );


#ifdef Q_WS_X11
    // required for multi-threaded viewer on linux:
    XInitThreads();
#endif

    mViewerWidget = new osgEarth::QtGui::ViewerWidget( &viewer );
    m3DWin = mMdiArea->addSubWindow(mViewerWidget);
    m3DWin->setWindowTitle(tr("三维窗口"));
    m3DWin->setGeometry(0, 0, 900, 600);
    m3DWin->setWindowState(Qt::WindowMaximized);
}

void myMainWindow::MouseXY(const QgsPoint &p)
{
    QString str = "当前坐标:";
    str.sprintf("当前坐标: 纬度:%f度, 经度:%f度", p.y(), p.x());
    statusBar()->showMessage(str);
}
