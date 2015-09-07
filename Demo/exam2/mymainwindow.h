#ifndef MYMAINWINDOW_H
#define MYMAINWINDOW_H

#include <QMainWindow>
#include <QMdiArea>
#include "qstring.h"
#include "qgsapplication.h"
#include "qgsmapcanvas.h"
#include "qgslayertreeview.h"
#include "qgslayertreemapcanvasbridge.h"
#include "qgsmaplayer.h"
#include "qgsmessagebar.h"
#include "qgsannotationitem.h"
#include "qgscustomlayerorderwidget.h"
#include "qgsmaptooldrawline.h"

#include "qgsmaptool.h"
#include "qgspoint.h"

#include <osgEarthQt/Common>
#include <osgEarthQt/ViewerWidget>


namespace Ui {
class myMainWindow;
}

class myMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit myMainWindow(QWidget *parent = 0);
    ~myMainWindow();

    static myMainWindow* smInstance;
    //Ui::examp2Class ui;
    QMdiArea *mMdiArea;
    QgsMapCanvas *mMapCanvas;
    QgsLayerTreeView *mLayerTreeView;
    //! Helper class that connects layer tree with map canvas
    QgsLayerTreeMapCanvasBridge *mLayerTreeCanvasBridge;
    //! Table of contents (legend) to order layers of the map
    void on_actionOpenPro_triggered();
    void on_actionSelectFeature_triggered();

private slots:

    void activeLayerChanged( QgsMapLayer* layer );

    void updateNewLayerInsertionPoint();

    void autoSelectAddedLayer( QList<QgsMapLayer*> layers );

    void toggleFilterLegendByMap();

    void on_actionOpenProject_triggered();

    void on_actionCloseProject_triggered();\

    void on_actionSaveProject_triggered();

    void on_actionSaveAsProject_triggered();

    void on_actionExit_triggered();

    void on_actionZoomIn_triggered();

    void on_actionZoomout_triggered();

    void on_actionMove_triggered();

    void on_actionFullMap_triggered();

    void on_actionRefresh_triggered();

    void on_actionPreviousView_triggered();

    void on_actionNextView_triggered();

    void on_actionAddGroupLayer_triggered();

    void on_actionAddVectorLayer_triggered();

    void on_actionAddRasterLayer_triggered();

    void on_actionAddMapService_triggered();

        void on_actionRemoveLayer_triggered();

    void on_actionPntSel_triggered();

    void on_actionSelectFeatures_triggered();

    void on_actionClear_triggered();

    void on_actionDist_triggered();

    void on_actionArea_triggered();

    void on_actionText_triggered();

    void on_actionSVG_triggered();

    void on_actionLine_triggered();

    void on_actionZone_triggered();

    void on_action3DView_triggered(bool bChecked);

    void MouseXY(const QgsPoint &p);

signals:
    void projectRead();


private:
    Ui::myMainWindow *ui;

    QDockWidget *mLayerTreeDock;
    QDockWidget *mLayerOrderDock;
    QgsCustomLayerOrderWidget *mMapLayerOrder;

    QAction* mActionFilterLegend;
    QgsMessageBar *mInfoBar;


    //地图交互工具
    QgsMapTool *pPanTool;
    QgsMapTool *pZoomInTool;
    QgsMapTool *pZoomOutTool;

    QgsMapTool *mMeasureDist;
    QgsMapTool *mMeasureArea;
    QgsMapTool *mTextAnnotationTool;
    QgsMapTool *mSvgAnnotationTool;


    //选择工具
    QgsMapTool *mSelectFeature;
    QgsMapTool *mSelectFeatures;
    QgsMapTool *mSelectPolygon;
    QgsMapTool *mSelectFreehand;
    QgsMapTool *mSelectRadius;
    //  QgsMaptool mSelectRectangle;

    //绘制工具
    QgsMapToolDrawLine *pDrawLineTool;

private:
    bool addProject( QString projectFile );
    void closeProject();
    void setFilterLegendByMapEnabled( bool enabled );
    void removeAllLayers();
    void removeAnnotationItems();
    void setupLayerTreeViewFromSettings();
    void initLayerTreeView();
    void setupConnections();

    QMdiSubWindow *m3DWin;
    QList<QgsAnnotationItem*> annotationItems();
    QgsLayerTreeView *layerTreeView();
    osgEarth::QtGui::ViewerWidget* mViewerWidget;

    void createMapTools();

    void create3DView();
    void create3DView2();

    // bool saveDirty();


};

#endif // MYMAINWINDOW_H
