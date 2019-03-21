//  Created by wangwei
//  Copyright © 2017-03-25 ExecuteSystem. All rights reserved.
#ifndef WTABLEVIEW_H
#define WTABLEVIEW_H

#include <QWidget>
#include <QScrollBar>
#include <QMap>
#include <QVector>

class WTableViewDelegate;
class WIndexPath
{
public:
    bool isValid() const;
    bool isNull() const;
    void setNull();
    explicit WIndexPath();
    explicit WIndexPath(int section,int row);
    int section;
    int row;
};


inline bool operator!=(const WIndexPath &index1,const WIndexPath &index2){
    return index1.section != index2.section || index1.row != index2.row;
}

inline bool operator==(const WIndexPath &index1,const WIndexPath &index2){
    return index1.section == index2.section && index1.row == index2.row;
}

inline bool operator<(const WIndexPath &index1,const WIndexPath &index2){
    if(index1.section < index2.section) return true;
    if(index1.section == index2.section){
        return index1.row < index2.row;
    }
    return false;
}

inline bool operator<=(const WIndexPath &index1,const WIndexPath &index2){
    if(index1.section < index2.section) return true;
    if(index1.section == index2.section){
        return index1.row <= index2.row;
    }
    return false;
}

inline bool operator>(const WIndexPath &index1,const WIndexPath &index2){
    if(index1.section > index2.section) return true;
    if(index1.section == index2.section){
        return index1.row > index2.row;
    }
    return false;
}

inline bool operator>=(const WIndexPath &index1,const WIndexPath &index2){
    if(index1.section > index2.section) return true;
    if(index1.section == index2.section){
        return index1.row >= index2.row;
    }
    return false;
}




class WTableViewCell : public QWidget
{
    Q_OBJECT

    friend class WTableView;
public:

    typedef enum :quint8{
        WTableViewCellSelectionStyleNone,
        WTableViewCellSelectionStyleBlue,
        WTableViewCellSelectionStyleGray,
        WTableViewCellSelectionStyleDefault
    }WTableViewCellSelectionStyle;


    WTableViewCellSelectionStyle selectionStyle;
    QColor backgroundColor;
    QColor selectedBackgroundColor;

    void hide();
    bool isHidden() const;
    void show();
    bool isSelected() const;
    void setSelectionStyle(WTableViewCellSelectionStyle style);
    WTableViewCell(QWidget *parent = 0,const QString &identifier = "");
    virtual ~WTableViewCell() {}
protected:
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

private: 
    bool hidden;
    QString identifier;
    bool selected;
    void setSelected(bool s);
    bool leftButtonPressed;
};

class WTableViewHeader : public QWidget
{
    Q_OBJECT
    friend class WTableView;
public:
    WTableViewHeader(QWidget *parent = 0,const QString &identifier = "");
    void hide();
    void show();
    bool isHidden() const;
    virtual ~WTableViewHeader() {}

protected:
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
private:
    bool hidden;
    QString identifier;
};


class WTableView : public QWidget
{
    Q_OBJECT
    friend class WTableViewCell;
    friend class WTableViewHeader;

public:
    enum WTableViewStyle{
        WTableViewStylePlain,
        WTableViewStyleGroup
    };

    explicit WTableView(QWidget *parent = 0,WTableViewStyle tableViewStyle = WTableViewStylePlain);

    WTableViewCell *dequeueReusableCellByIdentifier(const QString &identifier);
    WTableViewHeader *dequeueReusableHeaderByIdentifier(const QString &identifier);
    void scrollToY(int y);
    void setContentYOffset(quint32 y);
    void scrollToBottom();
    void scrollToTop();
    QSize  contentSize();
    int contentOffsetY();
    void setDelegate(WTableViewDelegate *delegate);
    WTableViewDelegate *getDelegate();
    bool isAllowSelection();
    void setAllowSelection(bool allow);
    bool isAllowMultipleSelection();
    void setAllowMultipleSelection(bool allow);
    void reloadRowAtIndexPath(const WIndexPath &indexPath);
    void insertRowAtIndexPath(const WIndexPath &indexPath);
    void insertSection(int section);
    void deleteRowAtIndexPath(const WIndexPath &indexPath);
    void selectedRowAtIndexPath(const WIndexPath &indexPath);
    void deselectRowAtIndexPath(const WIndexPath &indexPath);
    void setTableFooterView(QWidget *footerView);
    QWidget *getTableFooterView();
    WIndexPath indexPathForRowAtPoint(const QPoint &);// returns a invalid indexPath if point is outside of any row in the table
    WIndexPath indexPathForCell(WTableViewCell *cell);// returns a invalid indexPath if cell is not visible
    WTableViewCell *cellForRowAtIndexPath(const WIndexPath &indexPath);// returns empty QVector if cell is not visible or index path is out of range
    QVector<WIndexPath> indexPathsForVisibleRows();
    QVector<WTableViewCell *> visibleCells();
    QRect rectForRowAtIndexPath(const WIndexPath &indexPath);
    QRect rectForHeaderInSection(int section);
protected:
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;
    void enterEvent(QEvent *) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

signals:
    void tableViewScrollToY(int y);
public slots:
    void refreshContent();
    void reloadData();
private slots:
    void onScrollBarValueChanged(int value);
    void onSelectTableViewHeader(WTableViewHeader *);
    void onSelectTableViewCell(WTableViewCell *);
    void onTabbleViewDoubleClickCell(WTableViewCell *);
    void onTableViewCellPressed(WTableViewCell *);
private:
    void renderStartFromIndexPath(const WIndexPath &indexPath = WIndexPath());
    void updateContent();
    void cleanData();
    void storeCell(WTableViewCell *cell);
    void storeHeader(WTableViewHeader *header);
    void setCellSelectionState(WTableViewCell *cell,const WIndexPath &indexPath);
    QScrollBar *bar;
    QWidget *tableFooterView;
    QMap<QString,QVector<WTableViewCell*>*> cellsMap;
    QMap<QString,QVector<WTableViewHeader*>*> headersMap;
    WTableViewDelegate *delegate;
    QMap<WIndexPath,WTableViewCell *>showingCells;
    QMap<int,WTableViewHeader *>showingHeaders;
    QList<int>headerHeights;
    QList<int>headerYs;
    QList<QList<int> *>cellHeights;
    QList<QList<int> *>cellYs;
    int tableFooterViewY;
    QVector<WIndexPath>selectedIndexPaths;
    WTableViewStyle tableViewStyle;
    WIndexPath selectedIndexPath;
    int currentY;
    int contentHeight;
    bool allowSelection;
    bool allowMultipleSelection;
    bool isBarSliding;
};


#endif // WTABLEVIEW_H
