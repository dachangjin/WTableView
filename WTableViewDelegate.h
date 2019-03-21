//  Created by wangwei
//  Copyright © 2017-03-25 ExecuteSystem. All rights reserved.
#ifndef WTABLEVIEWDELEGATE_H
#define WTABLEVIEWDELEGATE_H

#include <QWidget>
#include "WTableView.h"



class WTableViewDelegate
{
public:
    WTableViewDelegate(){}
    virtual int numberOfSectionsInTableView(WTableView *tableView) = 0;
    virtual int tableViewNumberOfRowsInSection(WTableView *tableView,int section) = 0;
    virtual WTableViewCell *tableViewCellForRowAtIndex(WTableView *tableView,const WIndexPath &indexPath) = 0;
    virtual int tableViewHeightForRowAtIndexPath(WTableView *tableView,const WIndexPath &indexPath) = 0;
    virtual int tableViewHeightForHeaderInSection(int){return 0;}
    virtual WTableViewHeader *tableViewViewForHeaderInSection(WTableView *,int){return nullptr;}
    virtual void tableViewDidSelectHeaderAtSection(WTableView *,int){}
    virtual void tableViewDidSelectRowAtIndexPath(WTableView *,const WIndexPath &){}
    virtual void tableViewDidPressRowAtIndexPath(WTableView *,const WIndexPath &){}
    virtual void tableViewDidDeselectRowAtIndexPath(WTableView *,const WIndexPath &){}
    virtual void tableViewDoubleClickRowAtIndexPath(WTableView *,const WIndexPath &){}
    virtual void tableViewDidScrollToTop(WTableView *){}
    virtual void tableViewDidScrollToBottom(WTableView *){}
    virtual void tableViewDidScrollTo(WTableView *,int ){}
    virtual ~WTableViewDelegate(){}
};

#endif // WTABLEVIEWDELEGATE_H
