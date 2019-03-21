//  Created by wangwei
//  Copyright © 2017-03-25 ExecuteSystem. All rights reserved.
#include <QWheelEvent>
#include <QStyleOption>
#include <QPainter>
#include <QDebug>
#include <QMouseEvent>
#include "WTableView.h"
#include "WTableViewDelegate.h"


WTableView::WTableView(QWidget *parent,WTableViewStyle tableViewStyle) :
    QWidget(parent),
    tableFooterView(nullptr),
    delegate(nullptr),
    tableFooterViewY(INT_MAX),
    tableViewStyle(tableViewStyle),
    selectedIndexPath(WIndexPath(-1,-1)),
    currentY(0),
    contentHeight(0),
    allowSelection(true),
    allowMultipleSelection(false),
    isBarSliding(false)
{
    bar = new QScrollBar(this);
    bar->setSingleStep(1);
    bar->setMinimum(0);
    bar->setMaximum(0);
    bar->setFixedWidth(8);
    bar->hide();
    bar->setStyleSheet("QScrollBar:vertical"
                                "{"
                                "width:8px;"
                                "background:rgba(0,0,0,0);"
                                "margin:0px,0px,0px,0px;"
                                " }"
                                "QScrollBar::handle:vertical"
                                "{"
                                "width:8px;"
                                "background:rgba(0,0,0,0.25);"
                                "border-radius:4px;"
                                "min-height:20;"
                                "}"
                                "QScrollBar::handle:vertical:hover"
                                "{"
                                "width:8px;"
                                "background:rgba(0,0,0,0.5); "
                                " border-radius:4px;"
                                "min-height:20;"
                                "}"
                                "QScrollBar::sub-line:vertical"
                                "{"
                                "height:0px;width:0px;"
                                "subcontrol-position:top;"
                                "}"
                                "QScrollBar::add-line:vertical"
                                "{"
                                "height:0px;width:0px;"
                                "subcontrol-position:bottom;"
                                "}"
                       );
    connect(bar,&QScrollBar::valueChanged,this,&WTableView::onScrollBarValueChanged);
    connect(bar,&QScrollBar::sliderPressed,[this]{
        this->isBarSliding = true;
    });
    connect(bar,&QScrollBar::sliderReleased,[this]{
        this->isBarSliding = false;
    });
}

WTableViewCell *WTableView::dequeueReusableCellByIdentifier(const QString &identifier)
{
    if(cellsMap.contains(identifier)){
        QVector<WTableViewCell *> *widgets = cellsMap.value(identifier);
        for(WTableViewCell *w: *widgets){
            if(w->isHidden()){
                return w;
            }
        }
    }
    return nullptr;
}


WTableViewHeader *WTableView::dequeueReusableHeaderByIdentifier(const QString &identifier)
{
    if(headersMap.contains(identifier)){
        QVector<WTableViewHeader *> *widgets = headersMap.value(identifier);
        for(WTableViewHeader *w: *widgets){
            if(w->isHidden()){

                return w;
            }
        }
    }
    return nullptr;
}


void WTableView::scrollToY(int y)
{
    onScrollBarValueChanged(y);
}

void WTableView::setContentYOffset(quint32 y)
{
    bar->setValue(y);
}

void WTableView::scrollToBottom()
{
    bar->setValue(bar->maximum());
}

void WTableView::scrollToTop()
{
    bar->setValue(0);
}

QSize WTableView::contentSize()
{
    return QSize(this->width(),contentHeight);
}

int WTableView::contentOffsetY()
{
    return bar->value();
}

void WTableView::refreshContent()
{
    for(WTableViewCell *cell:showingCells.values()){
        cell->hide();
    }
    showingCells.clear();
    for(WTableViewHeader *header:showingHeaders.values()){
        header->hide();
    }
    showingHeaders.clear();
    updateContent();
}

void WTableView::reloadData()
{
    if(!delegate) return;
    selectedIndexPaths.clear();
    selectedIndexPath.setNull();
    for(QVector<WTableViewCell *> *cells:cellsMap.values()){
        for(WTableViewCell *cell: *cells){
            cell->hide();
//            delete cell;
            cell->deleteLater();
        }
        delete cells;
    }

    for(QVector<WTableViewHeader *> *headers:headersMap.values()){
        for(WTableViewHeader *header: *headers){
            header->hide();
//            delete header;
            header->deleteLater();
        }
        delete headers;
    }

    cellsMap.clear();
    headersMap.clear();
    showingCells.clear();
    showingHeaders.clear();
    updateContent();
}

void WTableView::setDelegate(WTableViewDelegate *delegate)
{
    this->delegate = delegate;
}

WTableViewDelegate *WTableView::getDelegate()
{
    return delegate;
}

bool WTableView::isAllowSelection(){
    return allowSelection;
}

void WTableView::setAllowSelection(bool allow){
    allowSelection = allow;
}

bool WTableView::isAllowMultipleSelection(){
    return allowMultipleSelection;
}

void WTableView::setAllowMultipleSelection(bool allow){
    if(allow) allowSelection = true;
    allowMultipleSelection = allow;
}

void WTableView::reloadRowAtIndexPath(const WIndexPath &indexPath)
{
    Q_ASSERT_X(indexPath.isValid(),"insertRowAtIndexPath","indexPath is invalid");
    Q_ASSERT_X(delegate,"reloadRowAtIndexPath","delagete is null");
    int section = delegate->numberOfSectionsInTableView(this);
    Q_ASSERT_X(indexPath.section < section ,"reloadRowAtIndexPath","indexPath section is out of range");
    int row = delegate->tableViewNumberOfRowsInSection(this,indexPath.section);
    Q_ASSERT_X(indexPath.row < row,"reloadRowAtIndexPath","indexPath row is out of range");

    int height = delegate->tableViewHeightForRowAtIndexPath(this,indexPath);
    int currentCellHeight = cellHeights.at(indexPath.section)->at(indexPath.row);
    int offset = height - currentCellHeight;

    if(offset != 0){
        QList<int> *heights = cellHeights.at(indexPath.section);
        Q_ASSERT_X(heights,"reloadRowAtIndexPath","can not find matched cell heights in section");
        Q_ASSERT_X(heights->size() > indexPath.row,"reloadRowAtIndexPath","indexPath section is out of range");
        heights->replace(indexPath.row,height);

        Q_ASSERT_X(cellYs.size() > indexPath.section,"reloadRowAtIndexPath","indexPath section is out of range");
        for(int i = indexPath.section; i < cellYs.size() ; i ++){
            if(i > indexPath.section){
                headerYs.replace(i,headerYs.at(i) + offset);
            }
            QList<int> *ys = cellYs.at(i);
            for(int j = 0; j < ys->size(); j ++){
                WIndexPath tempIndexPath(i,j);
                if(tempIndexPath > indexPath){
                    ys->replace(j,ys->at(j) + offset);
                }
            }
        }
        contentHeight += offset;
        if(tableFooterView){
            tableFooterViewY += offset;
        }
    }

    if(showingCells.contains(indexPath)){
        WTableViewCell *cell = showingCells.value(indexPath);
        cell->hide();
        storeCell(cell);
        showingCells.remove(indexPath);
    }

    renderStartFromIndexPath(indexPath);

    bar->move(this->width()-bar->width(),0);
    bar->resize(bar->width(),this->height());
    bar->setPageStep(this->height());
    if(contentHeight > this->height()){
        bar->setMaximum(contentHeight-this->height());
//            bar->show();
        bar->raise();
    }else {
        bar->setMaximum(0);
        bar->setValue(0);
        bar->hide();
    }

}

///to be tested
void WTableView::insertRowAtIndexPath(const WIndexPath &indexPath)
{
    Q_ASSERT_X(indexPath.isValid(),"insertRowAtIndexPath","indexPath is invalid");
    Q_ASSERT_X(delegate,"insertRowAtIndexPath","delagete is null");
    int sectionNumber = delegate->numberOfSectionsInTableView(this);
    Q_ASSERT_X(indexPath.section < sectionNumber,"insertRowAtIndexPath","indexPath section is out of range");
    int rowNumber = delegate->tableViewNumberOfRowsInSection(this,indexPath.section);
    Q_ASSERT_X(indexPath.row < rowNumber ,"insertRowAtIndexPath","indexPath row is out of range");
    Q_ASSERT_X(indexPath.section < headerHeights.size(),"insertRowAtIndexPath","indexPath section is out of range, should use insertSection(int section) func");

    int height = delegate->tableViewHeightForRowAtIndexPath(this,indexPath);
    int addedHeight = height;

    QList<int> *ys = cellYs.at(indexPath.section);
    QList<int> *heights = cellHeights.at(indexPath.section);
    if(ys->size() <= indexPath.row){
        ys->push_back(ys->last() + heights->last());
    }else {
        ys->insert(indexPath.row,ys->at(indexPath.row));
    }
    heights->insert(indexPath.row,height);
    for(int i = indexPath.section; i < cellYs.size() ; i ++){
        if(i > indexPath.section){
            headerYs.replace(i,headerYs.at(i) + height);
        }
        QList<int> *tys = cellYs.at(i);
        for(int j = 0; j < tys->size(); j ++){
            WIndexPath tempIndexPath(i,j);
            if(tempIndexPath > indexPath){
                tys->replace(j,tys->at(j) + height);
            }
        }
    }

    QMap<WIndexPath,WTableViewCell *> tempCells;
    for(WIndexPath idp:showingCells.keys()){
        if(idp.section == indexPath.section && idp.row >= indexPath.row){
            tempCells.insert(WIndexPath(indexPath.section,idp.row + 1),showingCells.value(idp));
            if(idp.row == indexPath.row){
                showingCells.remove(idp);
            }
        }
    }
    for(WIndexPath idp:tempCells.keys()){
        showingCells.insert(idp,tempCells.value(idp));
    }

    contentHeight += addedHeight;
    if(tableFooterView){
        tableFooterViewY += addedHeight;
    }

    renderStartFromIndexPath(indexPath);

    bar->move(this->width()-bar->width(),0);
    bar->resize(bar->width(),this->height());
    bar->setPageStep(this->height());
    if(contentHeight > this->height()){
        bar->setMaximum(contentHeight-this->height());
//        bar->show();
        bar->raise();
    }else {
        bar->setMaximum(0);
        bar->setValue(0);
        bar->hide();
    }

}

void WTableView::insertSection(int section)
{
    Q_ASSERT_X(section >= 0,"insertRowAtIndexPath","section < 0");
    Q_ASSERT_X(delegate,"insertRowAtIndexPath","delagete is null");
    int sectionNumber = delegate->numberOfSectionsInTableView(this);
    Q_ASSERT_X(section < sectionNumber,"insertRowAtIndexPath","indexPath section is out of range");
    int rowNumber = delegate->tableViewNumberOfRowsInSection(this,section);

    int headerY = 0;
    if(section != 0){
        headerY = headerYs.at(section - 1) + headerHeights.at(section - 1);
        QList<int> *rowYs = cellYs.at(section - 1);
        QList<int> *rowHs = cellHeights.at(section - 1);
        if(rowYs->size()){
            headerY = rowYs->last() + rowHs->last();
        }
    }
    headerYs.insert(section,headerY);

    int sectionHeight = delegate->tableViewHeightForHeaderInSection(section);

    int cellY = headerY + sectionHeight;

    headerHeights.insert(section,sectionHeight);
    int offset = sectionHeight;
    QList<int> *rowHeights = new QList<int>();
    QList<int> *rowYs = new QList<int>;
    for(int i = 0 ; i < rowNumber ; i ++){
        int rowHeight = delegate->tableViewHeightForRowAtIndexPath(this,WIndexPath(section,i));
        offset += rowHeight;
        rowHeights->push_back(rowHeight);

        rowYs->push_back(cellY);
        cellY += rowHeight;
    }
    this->cellHeights.insert(section,rowHeights);
    this->cellYs.insert(section,rowYs);

    for(int i = section + 1;i < headerYs.size(); i ++){
        headerYs.replace(i,headerYs.at(i) + offset);
    }

    for(int i = section + 1;i < this->cellYs.size();i ++){
        QList<int> *ys = cellYs.at(i);
        for(int j = 0;j < ys->size();j ++){
            ys->replace(j,ys->at(j) + offset);
        }
    }

    QMap<WIndexPath,WTableViewCell *> tempCells;
    for(WIndexPath idp:showingCells.keys()){
        if(idp.section >= section){
            tempCells.insert(WIndexPath(idp.section + 1,idp.row),showingCells.value(idp));
        }
    }

    for(WIndexPath idp:tempCells.keys()){
        showingCells.remove(WIndexPath(idp.section - 1,idp.row));
    }

    for(WIndexPath idp:tempCells.keys()){
        showingCells.insert(idp,tempCells.value(idp));
    }

    contentHeight += offset;
    if(tableFooterView){
        tableFooterViewY += offset;
    }

    renderStartFromIndexPath(WIndexPath(section,0));

    bar->move(this->width()-bar->width(),0);
    bar->resize(bar->width(),this->height());
    bar->setPageStep(this->height());
    if(contentHeight > this->height()){
        bar->setMaximum(contentHeight-this->height());
//        bar->show();
        bar->raise();
    }else {
        bar->setMaximum(0);
        bar->setValue(0);
        bar->hide();
    }


}

void WTableView::deleteRowAtIndexPath(const WIndexPath &indexPath)
{
    Q_ASSERT_X(indexPath.isValid(),"insertRowAtIndexPath","indexPath is invalid");
    Q_ASSERT_X(delegate,"insertRowAtIndexPath","delagete is null");
    int section = delegate->numberOfSectionsInTableView(this);
    Q_ASSERT_X(indexPath.section < section,"insertRowAtIndexPath","indexPath section is out of range");
    int row = delegate->tableViewNumberOfRowsInSection(this,indexPath.section);
    Q_ASSERT_X(indexPath.row < row ,"insertRowAtIndexPath","indexPath row is out of range");

    //TODO to be countinued;
}

void WTableView::selectedRowAtIndexPath(const WIndexPath &indexPath)
{
    if(!allowSelection) return;
    Q_ASSERT_X(indexPath.section <= headerHeights.size(),"selectedRowAtIndexPath","out of range");
    Q_ASSERT_X(indexPath.row <= cellHeights.at(indexPath.section)->size(),"selectedRowAtIndexPath","out of range");

    if(allowMultipleSelection){
        if(!selectedIndexPaths.contains(indexPath)){
            selectedIndexPaths.push_back(indexPath);
        }
    }else {
        if(indexPath != selectedIndexPath){
            selectedIndexPath = indexPath;
        }
    }
    WTableViewCell *cell = cellForRowAtIndexPath(indexPath);
    if(cell){
        cell->setSelected(true);
    }
}

void WTableView::deselectRowAtIndexPath(const WIndexPath &indexPath)
{
    if(!allowSelection) return;
    Q_ASSERT_X(indexPath.section <= headerHeights.size(),"deselectRowAtIndexPath","out of range");
    Q_ASSERT_X(indexPath.row <= cellHeights.at(indexPath.section)->size(),"deselectRowAtIndexPath","out of range");
    if(allowMultipleSelection){
        if(selectedIndexPaths.contains(indexPath)){
            selectedIndexPaths.removeAll(indexPath);
        }
    }else {
        if(indexPath == selectedIndexPath){
            selectedIndexPath = WIndexPath(-1,-1);
        }
    }
    WTableViewCell *cell = cellForRowAtIndexPath(indexPath);
    if(cell){
        cell->setSelected(false);
    }
}

void WTableView::setTableFooterView(QWidget *footerView)
{
    if(tableFooterView){
        contentHeight -= tableFooterView->height();
        delete tableFooterView;
    }
    tableFooterView = footerView;
    if(footerView){
        tableFooterView->setParent(this);
        tableFooterViewY = contentHeight;
        contentHeight += tableFooterView->height();
    }else {
        tableFooterViewY = INT_MAX;
    }

    renderStartFromIndexPath();
    bar->move(this->width()-bar->width(),0);
    bar->resize(bar->width(),this->height());
    bar->setPageStep(this->height());
    if(contentHeight > this->height()){
        bar->setMaximum(contentHeight-this->height());
//        bar->show();
        bar->raise();
    }else {
        bar->setMaximum(0);
        bar->setValue(0);
        bar->hide();
    }

}

QWidget *WTableView::getTableFooterView()
{
    return tableFooterView;
}

WIndexPath WTableView::indexPathForRowAtPoint(const QPoint &p)
{
    //to be test
    WIndexPath indexPath = WIndexPath(-1,-1);
    for(WTableViewCell *cell:showingCells){
        if(cell->geometry().contains(p)){
            indexPath = showingCells.key(cell,indexPath);
            break;
        }
    }
    return indexPath;
}

WIndexPath WTableView::indexPathForCell(WTableViewCell *cell)
{
    //to be test
     WIndexPath indexPath =  showingCells.key(cell,WIndexPath(-1,-1));
     return indexPath;
}

WTableViewCell *WTableView::cellForRowAtIndexPath(const WIndexPath &indexPath)
{
    return showingCells.value(indexPath);
}

QVector<WIndexPath> WTableView::indexPathsForVisibleRows()
{
    return showingCells.keys().toVector();
}

QVector<WTableViewCell *> WTableView::visibleCells()
{
    return showingCells.values().toVector();
}

QRect WTableView::rectForRowAtIndexPath(const WIndexPath &indexPath)
{
    if(cellYs.size() <= indexPath.section) return QRect();
    QList<int> *list = cellYs.at(indexPath.section);
    if(list->size() <= indexPath.row) return QRect();
    if(cellHeights.size() <= indexPath.section) return QRect();
    QList<int> *heights = cellHeights.at(indexPath.section);
    if(heights->size() <= indexPath.row) return QRect();
    int y = list->at(indexPath.row);
    int height = heights->at(indexPath.row);

    return QRect(0,y,width(),height);

}

QRect WTableView::rectForHeaderInSection(int section)
{
    if(headerYs.size() <= section) return QRect();
    if(headerHeights.size() <= section) return QRect();
    return QRect(0,headerYs.at(section),width(),headerHeights.at(section));
}


void WTableView::resizeEvent(QResizeEvent *event)
{
    updateContent();
    QWidget::resizeEvent(event);
}


void WTableView::wheelEvent(QWheelEvent *event)
{
    int step = bar->value() - event->delta() * 0.5;
    bar->setValue(step);
    QWidget::wheelEvent(event);
}

void WTableView::enterEvent(QEvent *e)
{
    if(contentHeight > this->height()){
        bar->setMaximum(contentHeight-this->height());
        bar->show();
        bar->raise();
    }
    QWidget::enterEvent(e);
}

void WTableView::leaveEvent(QEvent *e)
{
    if(this->isBarSliding == false){
        bar->hide();
    }
    QWidget::leaveEvent(e);
}

void WTableView::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    QWidget::paintEvent(event);
}

void WTableView::onScrollBarValueChanged(int value)
{
    if(currentY == value || value > bar->maximum() || value < 0) return;
    renderStartFromIndexPath();
    bar->raise();
    emit tableViewScrollToY(value);
    if(value == 0 && delegate){
        delegate->tableViewDidScrollToTop(this);
    }else if (value == bar->maximum() && delegate) {
        delegate->tableViewDidScrollToBottom(this);
    }
}

void WTableView::onSelectTableViewHeader(WTableViewHeader *header)
{
    int index = showingHeaders.key(header,-1);
    if(index >= 0){
        delegate->tableViewDidSelectHeaderAtSection(this,index);
    }
}

void WTableView::onSelectTableViewCell(WTableViewCell *cell)
{
    WIndexPath indexPath = showingCells.key(cell,WIndexPath(-1,-1));
    if(indexPath.section >=0 && allowSelection){
        if(allowMultipleSelection){
            if(selectedIndexPaths.contains(indexPath)){
                delegate->tableViewDidDeselectRowAtIndexPath(this,indexPath);
            }else {
                delegate->tableViewDidSelectRowAtIndexPath(this,indexPath);
            }
        }else {
            delegate->tableViewDidSelectRowAtIndexPath(this,indexPath);
        }

    }
}

void WTableView::onTabbleViewDoubleClickCell(WTableViewCell *cell)
{
    WIndexPath indexPath = showingCells.key(cell,WIndexPath(-1,-1));
    if(indexPath.section >= 0){
        delegate->tableViewDoubleClickRowAtIndexPath(this,indexPath);
    }
}

void WTableView::onTableViewCellPressed(WTableViewCell *cell)
{
    WIndexPath indexPath = showingCells.key(cell,WIndexPath(-1,-1));
    if(indexPath.section >=0 && allowSelection){
        if(allowMultipleSelection){
            if(selectedIndexPaths.contains(indexPath)){
                selectedIndexPaths.removeAll(indexPath);
//                renderStartFromIndexPath();
                if(cell){
                    cell->setSelected(false);
                }
            }else {
                selectedIndexPaths.push_back(indexPath);
//                renderStartFromIndexPath();
                if(cell){
                    cell->setSelected(true);
                }
            }
        }else {
            if(selectedIndexPath.isValid()){
                WTableViewCell *selectedCell = cellForRowAtIndexPath(selectedIndexPath);
                if(selectedCell){
                    selectedCell->setSelected(false);
                }
            }
            if(cell){
                cell->setSelected(true);
            }
            selectedIndexPath = indexPath;
        }
        delegate->tableViewDidPressRowAtIndexPath(this,indexPath);
    }
}

void WTableView::renderStartFromIndexPath(const WIndexPath &iP)
{
    if(!delegate) return;
    int value = bar->value();

    if(tableFooterView){
        tableFooterView->move(0,tableFooterViewY - value);
        if(!(tableFooterView->y() > this->height() || tableFooterView->y() + tableFooterView->height() < 0)){
            tableFooterView->show();
//            tableFooterView->setFixedSize(bar->isHidden() ?  this->width() :this->width()- bar->width(),tableFooterView->height());
            tableFooterView->setFixedSize(this->width(),tableFooterView->height());
        }else {
            tableFooterView->hide();
        }
    }

    QVector<WIndexPath> cellIndexPaths;

    for(WIndexPath indexPath:showingCells.keys()){
        WTableViewCell *cell = showingCells.value(indexPath);
        cell->hide();
        int y = cellYs.at(indexPath.section)->at(indexPath.row);
        int height = cellHeights.at(indexPath.section)->at(indexPath.row);
        cell->move(0,y - value);
        if(!(cell->y() > this->height() || cell->y() + cell->height() < 0)){
            cellIndexPaths.push_back(indexPath);
            setCellSelectionState(cell,indexPath);
//            cell->setFixedSize(bar->isHidden() ?  this->width() :this->width()- bar->width(),height);
            cell->setFixedSize(this->width(),height);
            cell->show();
        }else {
            showingCells.remove(indexPath);
        }
    }


    for(int i = iP.section;i < cellHeights.size(); i++){
        QList<int> *heights = cellHeights.at(i);
        if(heights->size()){
            for(int j = iP.row; j < heights->size(); j ++){
                WIndexPath indexPath(i,j);
                int y = cellYs.at(i)->at(j);
                int height = heights->at(j);
                if(((y - value) >= 0 && (y - value) < this->height()) || ((y - value + height) >=0 && (y - value + height) < this->height()) || ((y - value) <= 0 && (y - value + height) >= this->height())){
                    if(!cellIndexPaths.contains(indexPath)){
                        WTableViewCell *cell = delegate->tableViewCellForRowAtIndex(this,indexPath);
                        if(cell == nullptr) continue;// to be deleted
                        Q_ASSERT_X(cell,"WTableView","render-WTableViewCell");
                        storeCell(cell);
                        setCellSelectionState(cell,indexPath);
                        showingCells.insert(indexPath,cell);
//                        cell->setFixedSize(bar->isHidden() ?  this->width() :this->width()- bar->width(),height);
                        cell->setFixedSize(this->width(),height);
                        cell->move(0,y - value);
                        cell->setFixedHeight(height);
                        cell->show();
                    }
                }
            }
        }
    }

    QList<int> headerIndexs;
    for(int i:showingHeaders.keys()){
        WTableViewHeader *header = showingHeaders.value(i);
        header->hide();
        int y = headerYs.at(i);
        int height = headerHeights.at(i);
//        header->setFixedSize(bar->isHidden() ?  this->width() :this->width()- bar->width(),height);
        header->setFixedSize(this->width(),height);
        header->move(0,y - value);

        if(!(header->y() > this->height() || header->y() + header->height() < 0)){
            headerIndexs.push_back(i);
            header->show();
            header->raise();
            if(tableViewStyle == WTableViewStylePlain){
                if(!showingCells.isEmpty()){
                    WIndexPath indexPath = showingCells.firstKey();
                    if(i == indexPath.section){
                        int cellHeight = cellHeights.at(indexPath.section)->last();
                        int cellY = cellYs.at(indexPath.section)->last();
                        int offset = cellHeight + cellY - value - header->height();
                        if(offset > 0 && y - value < 0){
                            header->move(0,0);
                        }
                    }
                }
            }
        }else {
            if(tableViewStyle == WTableViewStylePlain && !showingCells.isEmpty()){
                WIndexPath indexPath = showingCells.firstKey();
                if(i == indexPath.section){
                    int cellHeight = cellHeights.at(indexPath.section)->last();
                    int cellY = cellYs.at(indexPath.section)->last();
                    int height = headerHeights.at(i);
                    int offset = cellHeight + cellY - value - height;
                    if(offset > 0 && y - value < 0){
                        header->move(0,0);
//                        header->setFixedSize(bar->isHidden() ?  this->width() :this->width()- bar->width(),height);
                        header->setFixedSize(this->width(),height);
                        headerIndexs.push_back(i);
                        header->show();
                        header->raise();
                    }else if(- offset <= header->height()){
                        header->move(0,offset);
//                        header->setFixedSize(bar->isHidden() ?  this->width() :this->width()- bar->width(),height);
                        header->setFixedSize(this->width(),height);
                        headerIndexs.push_back(i);
                        header->show();
                        header->raise();
                    }else {
                        showingHeaders.remove(i);
                    }
                }else {
                    showingHeaders.remove(i);
                }
            }else {
                showingHeaders.remove(i);
            }
        }
    }

    for(int i = iP.section;i < headerHeights.size() ;i ++){
        int y = headerYs.at(i);
        int height = headerHeights.at(i);
        if(!headerIndexs.contains(i)){

            if(((y - value) >= 0 && (y - value) < this->height()) || ((y - value + height) >=0 && (y - value + height) < this->height())){
                WTableViewHeader *header = delegate->tableViewViewForHeaderInSection(this,i);
                if(header == nullptr) continue;
                storeHeader(header);
//                Q_ASSERT_X(header,"WTableView","render-WTableViewHeader");
                showingHeaders.insert(i,header);
                header->move(0,y - value);
//                header->setFixedSize(bar->isHidden() ?  this->width() :this->width()- bar->width(),height);
                header->setFixedSize(this->width() ,height);
                header->show();
                header->raise();
            }else {
                if(tableViewStyle == WTableViewStylePlain){
                    if(!showingCells.isEmpty()){
                        WIndexPath indexPath = showingCells.firstKey();
                        if(i == indexPath.section){
                            WTableViewHeader *header = delegate->tableViewViewForHeaderInSection(this,i);
                            if(header == nullptr) continue;
                            storeHeader(header);
                            int cellHeight = cellHeights.at(indexPath.section)->last();
                            int cellY = cellYs.at(indexPath.section)->last();
                            int offset = cellHeight + cellY - value - header->height();
                            if(offset > 0 && y - value < 0){
                                header->move(0,0);
                                showingHeaders.insert(i,header);
//                                header->setFixedSize(bar->isHidden() ?  this->width() :this->width()- bar->width(),height);
                                header->setFixedSize(this->width(),height);
                                header->show();
                                header->raise();
                            }else if (-offset <= header->height()) {
                                header->move(0,offset);
//                                header->setFixedSize(bar->isHidden() ?  this->width() :this->width()- bar->width(),height);
                                header->setFixedSize(this->width(),height);
                                showingHeaders.insert(i,header);
                                header->show();
                                header->raise();
                            }
                        }
                    }
                }
            }
        }
    }
    currentY = value;

}

void WTableView::updateContent()
{
    if(!delegate) return;
    cleanData();
    int y = 0;
    int section = delegate->numberOfSectionsInTableView(this);
    for(int i = 0; i < section ; i ++){
        int sectionHeight = delegate->tableViewHeightForHeaderInSection(i);
        headerHeights.push_back(sectionHeight);
        headerYs.push_back(y);
        y += sectionHeight;

        int rows = delegate->tableViewNumberOfRowsInSection(this,i);
        QList<int> *sectionCellheights = new QList<int>();
        QList<int> *sectionCellYs = new QList<int>();
        for(int j = 0;j < rows; j ++){
            int rowHeight = delegate->tableViewHeightForRowAtIndexPath(this,WIndexPath(i,j));
            sectionCellheights->push_back(rowHeight);

            sectionCellYs->push_back(y);
            y += rowHeight;
//            qDebug()<<"rowHeight:"<<rowHeight<<"===contentHeight:"<<y;

        }
        cellHeights.push_back(sectionCellheights);
        cellYs.push_back(sectionCellYs);

    }

    contentHeight = y;
    if(tableFooterView){
        tableFooterViewY = contentHeight;
        contentHeight += tableFooterView->height();
    }


    renderStartFromIndexPath();
    bar->move(this->width()-bar->width(),0);
    bar->resize(bar->width(),this->height());
    bar->setPageStep(this->height());
    if(contentHeight > this->height()){
        bar->setMaximum(contentHeight-this->height());
//        bar->show();
        bar->raise();
    }else {
        bar->setMaximum(0);
        bar->setValue(0);
        bar->hide();
    }
}

void WTableView::cleanData()
{
    for(QList<int> *heights : cellHeights){
        heights->clear();
        delete heights;
    }
    for(QList<int> *ys : cellYs){
        ys->clear();
        delete ys;
    }
    cellHeights.clear();
    headerHeights.clear();
    cellYs.clear();
    headerYs.clear();
}

void WTableView::storeCell(WTableViewCell *cell)
{
    QString identifier = cell->identifier;
    if(cellsMap.contains(identifier)){

        QVector<WTableViewCell *> *cells = cellsMap.value(identifier);
        if(!cells->contains(cell)){
            cells->push_back(cell);
        }
    }else {
        QVector<WTableViewCell *> *cells = new QVector<WTableViewCell *>();
        cells->append(cell);
        cellsMap.insert(identifier,cells);
    }
}

void WTableView::storeHeader(WTableViewHeader *header)
{
    QString identifier = header->identifier;
    if(headersMap.contains(identifier)){

        QVector<WTableViewHeader *> *headers = headersMap.value(identifier);
        if(!headers->contains(header)){
            headers->push_back(header);
        }
    }else {
        QVector<WTableViewHeader *> *headers = new QVector<WTableViewHeader *>();
        headers->append(header);
        headersMap.insert(identifier,headers);
    }
}

void WTableView::setCellSelectionState(WTableViewCell *cell, const WIndexPath &indexPath)
{
    if(allowSelection){
        if(allowMultipleSelection){
            if(selectedIndexPaths.contains(indexPath)){
                cell->setSelected(true);
            }else {
                cell->setSelected(false);
            }
        }else {
            if(selectedIndexPath == indexPath){
                cell->setSelected(true);
            }else {
                cell->setSelected(false);
            }
        }
    }
}


WTableViewHeader::WTableViewHeader(QWidget *parent, const QString &identifier) : QWidget(parent),identifier(identifier) {
    hidden = false;
}

void WTableViewHeader::hide(){
    hidden = true;
    QWidget::hide();
}

void WTableViewHeader::show(){
    hidden = false;
    QWidget::show();
}

bool WTableViewHeader::isHidden() const{
    return hidden;
}

void WTableViewHeader::mousePressEvent(QMouseEvent *)
{
    if(parent()->inherits("WTableView")){
        WTableView *tableView = static_cast<WTableView *>(parent());
        tableView->onSelectTableViewHeader(this);
    }
}

void WTableViewHeader::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    QWidget::paintEvent(event);
}

void WTableViewCell::hide(){
    hidden = true;
    QWidget::hide();
}

bool WTableViewCell::isHidden() const{
    return hidden;
}

void WTableViewCell::show(){
    hidden = false;
    QWidget::show();
}

bool WTableViewCell::isSelected() const
{
    return selected;
}

void WTableViewCell::setSelectionStyle(WTableViewCell::WTableViewCellSelectionStyle style)
{
    this->selectionStyle = style;
}

WTableViewCell::WTableViewCell(QWidget *parent, const QString &identifier) :
    QWidget(parent),
    selectionStyle(WTableViewCellSelectionStyleGray),
    hidden(false),
    identifier(identifier),
    leftButtonPressed(false) {
}

void WTableViewCell::mousePressEvent(QMouseEvent *event)
{
    if(parent()->inherits("WTableView")){
        WTableView *tableView = static_cast<WTableView *>(parent());
        if(event->type() == QEvent::MouseButtonPress && event->button() == Qt::LeftButton){
            tableView->onTableViewCellPressed(this);
            leftButtonPressed = true;
        }else if (event->type() == QEvent::MouseButtonDblClick && event->button() == Qt::LeftButton) {
            tableView->onTabbleViewDoubleClickCell(this);
        }
    }
    QWidget::mousePressEvent(event);
}

void WTableViewCell::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton){
        if(leftButtonPressed) {
            if(rect().contains(event->pos())){
                WTableView *tableView = static_cast<WTableView *>(parent());
                tableView->onSelectTableViewCell(this);
            }
        }
        leftButtonPressed = false;
    }
    QWidget::mouseReleaseEvent(event);

}

void WTableViewCell::setSelected(bool s)
{
    setAutoFillBackground(true);
    selected = s;
    QPalette p(palette());
    if(selectionStyle != WTableViewCellSelectionStyleNone){
        if(selected){
            if(selectedBackgroundColor.isValid()){
                p.setColor(QPalette::Background,selectedBackgroundColor);
            }else {
                switch (selectionStyle) {
                case WTableViewCellSelectionStyleBlue:
                    p.setColor(QPalette::Background,Qt::blue);
                    break;
                case WTableViewCellSelectionStyleGray:
                case WTableViewCellSelectionStyleDefault:
                    p.setColor(QPalette::Background,Qt::gray);
                    break;
                default:
                    break;
                }
            }
        }else {
            if(backgroundColor.isValid()){
                p.setColor(QPalette::Background,backgroundColor);
            }else {
                p.setColor(QPalette::Background,Qt::white);
            }

        }
        this->setPalette(p);
    }else {
        if(backgroundColor.isValid()){
            p.setColor(QPalette::Background,backgroundColor);
            this->setPalette(p);
        }else {
            p.setColor(QPalette::Background,Qt::white);
            this->setPalette(p);
        }
    }
}

bool WIndexPath::isValid() const
{
    return row >= 0 && section >=0;
}

bool WIndexPath::isNull() const
{
    return row == -1 && section == -1;
}

void WIndexPath::setNull()
{
    row = -1;
    section = -1;
}

WIndexPath::WIndexPath():section(0),row(0){}

WIndexPath::WIndexPath(int section, int row):section(section),row(row) {}
