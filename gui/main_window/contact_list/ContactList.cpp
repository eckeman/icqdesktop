#include "stdafx.h"

#include "Common.h"
#include "ContactList.h"
#include "ContactItem.h"
#include "ContactListModel.h"
#include "ContactListItemDelegate.h"
#include "ContactListItemRenderer.h"
#include "RecentsItemRenderer.h"
#include "RecentsModel.h"
#include "RecentItemDelegate.h"
#include "UnknownsModel.h"
#include "UnknownItemDelegate.h"
#include "SearchModelDLG.h"
#include "SettingsTab.h"
#include "../MainWindow.h"
#include "../contact_list/ChatMembersModel.h"
#include "../livechats/LiveChatsModel.h"
#include "../livechats/LiveChatItemDelegate.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../types/contact.h"
#include "../../types/typing.h"
#include "../../controls/ContextMenu.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../utils/gui_coll_helper.h"

namespace
{
    const int balloon_size = 20;
    const int unreads_padding = 6;
    const int unreads_minimum_extent_ = 20;
    const auto unreadsFont = ContactList::dif(Fonts::defaultAppFontFamily(), Fonts::FontWeight::Semibold, 13);
    const int LIVECHATS_TITLE_HEIGHT = 64;
    const int autoscroll_offset_recents = 68;
    const int autoscroll_offset_cl = 44;
    const int autoscroll_speed_pixels = 10;
    const int autoscroll_timeout = 50;

    const QString CL_STYLE = QString(
        "border-right-style: solid; border-right-color: #dadada; border-right-width: 1dip;"
    );

    const QString CL_TRANSPARENT = QString(
        "background-color: transparent;"
        "border: none;"
    );

    QMap<QString, QVariant> makeData(const QString& _command, const QString& _aimid = QString())
    {
        QMap<QString, QVariant> result;
        result["command"] = _command;
        result["contact"] = _aimid;
        return result;
    }
}

namespace Ui
{
    AddContactButton::AddContactButton(QWidget* _parent)
    : QWidget(_parent)
    , painter_(0)
    , hover_(false)
    , select_(false)
    {
    }

    void AddContactButton::paintEvent(QPaintEvent*)
    {
        if (!painter_)
        {
            painter_ = new QPainter(this);
            painter_->setRenderHint(QPainter::Antialiasing);
        }

        painter_->begin(this);
        ::ContactList::ViewParams viewParams;
        ::ContactList::RenderServiceContact(*painter_, hover_, select_, QT_TRANSLATE_NOOP("contact_list","Add contact"), Data::ContactType::ADD_CONTACT, 0, viewParams);
        painter_->end();
    }

    void AddContactButton::enterEvent(QEvent* _e)
    {
        hover_ = true;
        update();
        return QWidget::enterEvent(_e);
    }

    void AddContactButton::leaveEvent(QEvent* _e)
    {
        hover_ = false;
        update();
        return QWidget::leaveEvent(_e);
    }

    void AddContactButton::mousePressEvent(QMouseEvent* _e)
    {
        select_ = true;
        update();
        return QWidget::mousePressEvent(_e);
    }

    void AddContactButton::mouseReleaseEvent(QMouseEvent* _e)
    {
        select_ = false;
        update();
        emit clicked();
        return QWidget::mouseReleaseEvent(_e);
    }


    SearchInAllChatsButton::SearchInAllChatsButton(QWidget* _parent)
    : QWidget(_parent)
        , painter_(0)
        , hover_(false)
        , select_(false)
    {
    }

    void SearchInAllChatsButton::paintEvent(QPaintEvent*)
    {
        if (!painter_)
        {
            painter_ = new QPainter(this);
            painter_->setRenderHint(QPainter::Antialiasing);
        }

        auto vMainLayout_ = new QVBoxLayout(this);
        vMainLayout_->setContentsMargins(0, 0, 0, 0);
        vMainLayout_->setSpacing(0);

        auto horLineWidget_ = new QWidget(this);
        horLineWidget_->setFixedHeight(Utils::scale_value(1));
        horLineWidget_->setStyleSheet(QString("background-color: #dadada;"));
        horLineWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        vMainLayout_->addSpacing(0);

        auto horLine_ = new QWidget(this);
        auto horLineLayout_ = new QHBoxLayout(horLine_);
        auto findInAllChatsHeight = ::ContactList::SearchInAllChatsHeight();

        horLineLayout_->setContentsMargins(Utils::scale_value(24), 0, Utils::scale_value(24), findInAllChatsHeight);
        horLineLayout_->addWidget(horLineWidget_);
        vMainLayout_->addWidget(horLine_);

        painter_->begin(this);
        ::ContactList::ViewParams viewParams;
        ::ContactList::RenderServiceContact(*painter_, hover_, select_, QT_TRANSLATE_NOOP("contact_list","Search in all chats"), Data::ContactType::SEARCH_IN_ALL_CHATS, 0, viewParams);
        painter_->end();
    }

    void SearchInAllChatsButton::enterEvent(QEvent* _e)
    {
        hover_ = true;
        update();
        return QWidget::enterEvent(_e);
    }

    void SearchInAllChatsButton::leaveEvent(QEvent* _e)
    {
        hover_ = false;
        update();
        return QWidget::leaveEvent(_e);
    }

    void SearchInAllChatsButton::mousePressEvent(QMouseEvent* _e)
    {
        select_ = true;
        update();
        return QWidget::mousePressEvent(_e);
    }

    void SearchInAllChatsButton::mouseReleaseEvent(QMouseEvent* _e)
    {
        select_ = false;
        update();
        emit clicked();
        return QWidget::mouseReleaseEvent(_e);
    }


    SearchInChatLabel::SearchInChatLabel(QWidget* _parent)
        : QWidget(_parent)
        , painter_(0)
    {
    }

    void SearchInChatLabel::paintEvent(QPaintEvent*)
    {
        if (!painter_)
        {
            painter_ = new QPainter(this);
            painter_->setRenderHint(QPainter::Antialiasing);
        }

        auto vMainLayout_ = new QVBoxLayout(this);
        vMainLayout_->setContentsMargins(0, 0, 0, 0);
        vMainLayout_->setSpacing(0);

        painter_->begin(this);
        ::ContactList::ViewParams viewParams;

        auto searchModel =  qobject_cast<Logic::SearchModelDLG*>(Logic::getCurrentSearchModel(Logic::MembersWidgetRegim::CONTACT_LIST));
        auto chatAimid = searchModel->getDialogAimid();
        auto text = QString::fromWCharArray(L"SEARCH IN \u00AB") + Logic::getContactListModel()->getDisplayName(chatAimid).toUpper() + QString::fromWCharArray(L"\u00BB");
        ::ContactList::RenderServiceItem(*painter_, text, false /* renderState */, false /* drawLine */, viewParams);
        painter_->end();
    }


    EmptyIgnoreListLabel::EmptyIgnoreListLabel(QWidget* _parent)
    : QWidget(_parent)
    , painter_(0)
    {
    }

    void EmptyIgnoreListLabel::paintEvent(QPaintEvent*)
    {
        if (!painter_)
        {
            painter_ = new QPainter(this);
            painter_->setRenderHint(QPainter::Antialiasing);
        }

        painter_->begin(this);

        ::ContactList::ViewParams viewParams;
        ::ContactList::RenderServiceContact(
            *painter_,
            false /* Hover_ */,
            false /* Select_ */,
            QT_TRANSLATE_NOOP("sidebar", "You have no ignored contacts"),
            Data::ContactType::EMPTY_IGNORE_LIST,
            0,
            viewParams
        );
        painter_->end();
    }

    RCLEventFilter::RCLEventFilter(ContactList* _cl)
    : QObject(_cl)
    , cl_(_cl)
    {

    }

    bool RCLEventFilter::eventFilter(QObject* _obj, QEvent* _event)
    {
        if (_event->type() == QEvent::Gesture)
        {
            QGestureEvent* guesture  = static_cast<QGestureEvent*>(_event);
            if (QGesture *tapandhold = guesture->gesture(Qt::TapAndHoldGesture))
            {
                if (tapandhold->hasHotSpot() && tapandhold->state() == Qt::GestureFinished)
                {
                    cl_->triggerTapAndHold(true);
                    guesture->accept(Qt::TapAndHoldGesture);
                }
            }
        }
        if (_event->type() == QEvent::DragEnter || _event->type() == QEvent::DragMove)
        {
            Utils::InterConnector::instance().getMainWindow()->closeGallery();
            Utils::InterConnector::instance().getMainWindow()->activate();
            QDropEvent* de = static_cast<QDropEvent*>(_event);
            if (de->mimeData() && de->mimeData()->hasUrls())
            {
                de->acceptProposedAction();
                cl_->dragPositionUpdate(de->pos());
            }
            else
            {
                de->setDropAction(Qt::IgnoreAction);
            }
            return true;
        }
        if (_event->type() == QEvent::DragLeave)
        {
            cl_->dragPositionUpdate(QPoint());
            return true;
        }
        if (_event->type() == QEvent::Drop)
        {
            QDropEvent* e = static_cast<QDropEvent*>(_event);
            const QMimeData* mimeData = e->mimeData();
            QList<QUrl> urlList;
            if (mimeData->hasUrls())
            {
                urlList = mimeData->urls();
            }

            cl_->dropFiles(e->pos(), urlList);
            e->acceptProposedAction();
            cl_->dragPositionUpdate(QPoint());
        }

        if (_event->type() == QEvent::MouseButtonDblClick)
        {
            _event->ignore();
            return true;
        }

        if (_event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* e = static_cast<QMouseEvent*>(_event);
            if (e->button() == Qt::LeftButton)
            {
                cl_->triggerTapAndHold(false);
            }
        }

        return QObject::eventFilter(_obj, _event);
    }

    RecentsButton::RecentsButton(QWidget* _parent)
    : QPushButton(_parent)
    , painter_(new QPainter(this))
    {
        setObjectName("recentsButton");
        setCheckable(true);
    }

    void RecentsButton::paintEvent(QPaintEvent* _e)
    {
        QPushButton::paintEvent(_e);

        const auto unreads = (Logic::getRecentsModel()->totalUnreads() + Logic::getUnknownsModel()->totalUnreads());
        if (unreads > 0)
        {
            const auto text = (unreads > 99) ? QString("99+") : QVariant(unreads).toString();

            static QFontMetrics m(unreadsFont.font());

            const auto unreadsRect = m.tightBoundingRect(text);
            const auto firstChar = text[0];
            const auto lastChar = text[text.size() - 1];
            const auto unreadsWidth = (unreadsRect.width() + m.leftBearing(firstChar) + m.rightBearing(lastChar));

            auto balloonWidth = unreadsWidth;
            const auto isLongText = (text.length() > 1);
            if (isLongText)
            {
                balloonWidth += Utils::scale_value(unreads_padding * 2);
            }
            else
            {
                balloonWidth = Utils::scale_value(unreads_minimum_extent_);
            }

            if (painter_->begin(this))
            {
                painter_->setPen(QColor("#579e1c"));
                painter_->setRenderHint(QPainter::Antialiasing);
                const auto radius = Utils::scale_value(balloon_size / 2);
                painter_->setBrush(QColor("#579e1c"));
                int x = (width() / 2) + Utils::scale_value(balloon_size / 2) - radius;
                int y = (height() / 2) - Utils::scale_value(balloon_size / 2) - radius;
                painter_->drawRoundedRect(x, y, balloonWidth, Utils::scale_value(balloon_size), radius, radius);

                painter_->setFont(unreadsFont.font());
                painter_->setPen(Qt::white);
                const auto textX = x + ((balloonWidth - unreadsWidth) / 2);
                painter_->drawText(textX, (height() / 2) - Utils::scale_value(balloon_size / 2) + radius / 2, text);
                painter_->end();
            }
        }
    }

    ContactList::ContactList(QWidget* _parent, Logic::MembersWidgetRegim _regim, Logic::ChatMembersModel* _chatMembersModel)
    : QWidget(_parent)
    , currentTab_(RECENTS)
    , unknownsDelegate_(new Logic::UnknownItemDelegate(this))
    , recentsDelegate_(new Logic::RecentItemDelegate(this))
    , clDelegate_(new Logic::ContactListItemDelegate(this, _regim))
    , recentsButton_(new RecentsButton(this))
    , popupMenu_(nullptr)
    , regim_(_regim)
    , chatMembersModel_(_chatMembersModel)
    , noContactsYet_(nullptr)
    , noRecentsYet_(nullptr)
    , noSearchResults_(nullptr)
    , searchSpinner_(nullptr)
    , noContactsYetShown_(false)
    , noRecentsYetShown_(false)
    , noSearchResultsShown_(false)
    , searchSpinnerShown_(false)
    , tapAndHold_(false)
    , listEventFilter_(new RCLEventFilter(this))
    , liveChatsDelegate_(new Logic::LiveChatItemDelegate(this))
    , fixedItemWidth_(-1)
    , pictureOnlyView_(false)
    , scrolledView_(0)
    , scrollMultipler_(1)
    , isSearchInDialog_(false)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        setStyleSheet(Utils::LoadStyle(":/main_window/contact_list/contact_list.qss"));
        auto mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(0);
        mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        stackedWidget_ = new QStackedWidget(this);
        Utils::ApplyStyle(stackedWidget_, CL_STYLE);

        QPalette Pal(palette());
        Pal.setColor(QPalette::Background, Ui::CommonStyle::getFrameTransparency());
        stackedWidget_->setAutoFillBackground(true);
        stackedWidget_->setPalette(Pal);

        recentsPage_ = new QWidget();
        recentsLayout_ = new QVBoxLayout(recentsPage_);
        recentsLayout_->setSpacing(0);
        recentsLayout_->setContentsMargins(0, 0, 0, 0);
        recentsView_ = CreateFocusableViewAndSetTrScrollBar(recentsPage_);
        recentsView_->setFrameShape(QFrame::NoFrame);
        recentsView_->setLineWidth(0);
        recentsView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        recentsView_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        recentsView_->setAutoScroll(false);
        recentsView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        recentsView_->setUniformItemSizes(false);
        recentsView_->setBatchSize(50);
        recentsView_->setProperty("ContactListWidget", QVariant(true));
        recentsView_->setCursor(Qt::PointingHandCursor);
        recentsView_->setMouseTracking(true);
        recentsView_->setAcceptDrops(true);
        recentsView_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

        recentsLayout_->addWidget(recentsView_);
        stackedWidget_->addWidget(recentsPage_);
        Testing::setAccessibleName(recentsView_, "RecentView");

        contactListPage_ = new QWidget();
        contactListLayout_ = new QVBoxLayout(contactListPage_);
        contactListLayout_->setSpacing(0);
        contactListLayout_->setContentsMargins(0, 0, 0, 0);
        contactListView_ = CreateFocusableViewAndSetTrScrollBar(contactListPage_);
        contactListView_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        contactListView_->setFrameShape(QFrame::NoFrame);
        contactListView_->setSpacing(0);
        contactListView_->setModelColumn(0);
        contactListView_->setUniformItemSizes(false);
        contactListView_->setBatchSize(50);
        contactListView_->setProperty("ContactListWidget", QVariant(true));
        contactListView_->setCursor(Qt::PointingHandCursor);
        contactListView_->setMouseTracking(true);
        contactListView_->setAcceptDrops(true);
        contactListView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        contactListView_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        contactListView_->setAutoScroll(false);
        contactListView_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

        if (_regim == Logic::MembersWidgetRegim::CONTACT_LIST)
        {
            AddContactButton* addButton = new AddContactButton(this);
            Testing::setAccessibleName(addButton, "AddNewContactButton");
            addButton->setContentsMargins(0, 0, 0, 0);
            addButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
            addButton->setFixedHeight(::ContactList::ContactItemHeight());
            connect(addButton, SIGNAL(clicked()), this, SIGNAL(addContactClicked()), Qt::QueuedConnection);
            contactListLayout_->addWidget(addButton);
        }

        emptyIgnoreListLabel_ = new EmptyIgnoreListLabel(this);
        emptyIgnoreListLabel_->setContentsMargins(0, 0, 0, 0);
        emptyIgnoreListLabel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        emptyIgnoreListLabel_->setFixedHeight(::ContactList::ContactItemHeight());
        contactListLayout_->addWidget(emptyIgnoreListLabel_);
        emptyIgnoreListLabel_->setVisible(false);

        contactListLayout_->addWidget(contactListView_);
        stackedWidget_->addWidget(contactListPage_);

        {
            liveChatsPage_ = new QWidget();
            auto livechatsLayout = new QVBoxLayout(liveChatsPage_);
            livechatsLayout->setSpacing(0);
            livechatsLayout->setContentsMargins(0, 0, 0, 0);

            QLabel* livechatsTitle = new QLabel(contactListPage_);
            Utils::grabTouchWidget(livechatsTitle);
            livechatsTitle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            livechatsTitle->setObjectName("liveChatsTitle");
            livechatsTitle->setText(QT_TRANSLATE_NOOP("livechats","Live chats"));
            livechatsTitle->setFixedHeight(Utils::scale_value(LIVECHATS_TITLE_HEIGHT));
            livechatsLayout->addWidget(livechatsTitle);

            liveChatsView_ = CreateFocusableViewAndSetTrScrollBar(liveChatsPage_);
            liveChatsView_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            liveChatsView_->setFrameShape(QFrame::NoFrame);
            liveChatsView_->setFrameShadow(QFrame::Plain);
            liveChatsView_->setFocusPolicy(Qt::NoFocus);
            liveChatsView_->setLineWidth(0);
            liveChatsView_->setBatchSize(50);
            liveChatsView_->setProperty("ContactListWidget", QVariant(true));
            liveChatsView_->setCursor(Qt::PointingHandCursor);
            liveChatsView_->setMouseTracking(true);
            liveChatsView_->setAcceptDrops(true);
            liveChatsView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            liveChatsView_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
            liveChatsView_->setAutoScroll(false);

            livechatsLayout->addWidget(liveChatsView_);
            stackedWidget_->addWidget(liveChatsPage_);
        }

        {
            searchPage_ = new QWidget();
            auto searchWithButtonLayout_ = new QVBoxLayout(searchPage_);
            searchWithButtonLayout_->setSpacing(0);
            searchWithButtonLayout_->setContentsMargins(0, 0, 0, 0);

            searchInChatLabel_ = new SearchInChatLabel();
            searchInChatLabel_->setContentsMargins(0, 0, 0, 0);
            searchInChatLabel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
            searchInChatLabel_->setFixedHeight(::ContactList::GetRecentsParams(Logic::MembersWidgetRegim::CONTACT_LIST).serviceItemHeight().px());
            searchWithButtonLayout_->addWidget(searchInChatLabel_);
            searchInChatLabel_->setVisible(false);

            auto searchHost_ = new QWidget();
            searchLayout_ = new QVBoxLayout(searchHost_);
            searchLayout_->setSpacing(0);
            searchLayout_->setContentsMargins(0, 0, 0, 0);
            searchView_ = CreateFocusableViewAndSetTrScrollBar(searchPage_);
            searchView_->setFrameShape(QFrame::NoFrame);
            searchView_->setLineWidth(0);
            searchView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            searchView_->setProperty("ContactListWidget", QVariant(true));
            searchView_->setMouseTracking(true);
            searchView_->setCursor(Qt::PointingHandCursor);
            searchView_->setAcceptDrops(true);
            searchView_->setAttribute(Qt::WA_MacShowFocusRect, false);
            searchView_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
            searchLayout_->addWidget(searchView_);
            searchWithButtonLayout_->addWidget(searchHost_);

            stackedWidget_->addWidget(searchPage_);
            mainLayout->addWidget(stackedWidget_);

            searchInAllButton_ = new SearchInAllChatsButton();
            Testing::setAccessibleName(searchInAllButton_, "SearchInAllChatsButton");
            searchInAllButton_->setContentsMargins(0, 0, 0, 0);
            searchInAllButton_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
            searchInAllButton_->setFixedHeight(::ContactList::SearchInAllChatsHeight());
            connect(searchInAllButton_, &SearchInAllChatsButton::clicked, this, &ContactList::onDisableSearchInDialogButton, Qt::QueuedConnection);
            searchWithButtonLayout_->addWidget(searchInAllButton_);
            searchInAllButton_->setVisible(false);
        }

        buttonsFrame_ = new QFrame(this);
        buttonsFrame_->setObjectName("bottomPanel");
        buttonsFrame_->setFrameShape(QFrame::NoFrame);
        buttonsFrame_->setFrameShadow(QFrame::Sunken);
        buttonsFrame_->setLineWidth(0);
        Testing::setAccessibleName(buttonsFrame_, "frame_");

        auto buttonsLayout = new QHBoxLayout(buttonsFrame_);

        buttonsLayout->setSpacing(0);
        buttonsLayout->setContentsMargins(0, 0, 0, 0);
        {
            auto horizontal_spacer = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
            buttonsLayout->addItem(horizontal_spacer);
        }
        clTabButton_ = new QPushButton(buttonsFrame_);
        clTabButton_->setObjectName("contactListButton");
        clTabButton_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        clTabButton_->setCheckable(true);
        clTabButton_->setFlat(false);
        Testing::setAccessibleName(clTabButton_, "AllTabButton");

        livechatsButton_ = new QPushButton(buttonsFrame_);
        livechatsButton_->setObjectName("liveChatsButton");
        livechatsButton_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        livechatsButton_->setCheckable(true);
        livechatsButton_->setFlat(false);
        Testing::setAccessibleName(livechatsButton_, "livechatsButton_");

        buttonsLayout->addWidget(clTabButton_);

        settingsTabButton_ = new QPushButton(buttonsFrame_);
        settingsTabButton_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        settingsTabButton_->setCheckable(true);
        settingsTabButton_->setFlat(false);
        settingsTabButton_->setObjectName("settingsButton");
        Testing::setAccessibleName(settingsTabButton_, "settingsTabButton_");
        buttonsLayout->addWidget(settingsTabButton_);

        auto horizontal_spacer_buttons_right_ = new QSpacerItem(Utils::scale_value(28), 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
        buttonsLayout->addItem(horizontal_spacer_buttons_right_);
        {
            auto horizontal_spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
            buttonsLayout->addItem(horizontal_spacer);
        }
        mainLayout->addWidget(buttonsFrame_);
        auto vertical_spacer_ = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Minimum);
        mainLayout->addItem(vertical_spacer_);

        clTabButton_->setText(QString());
        settingsTabButton_->setText(QString());

        stackedWidget_->setCurrentIndex(0);
        QMetaObject::connectSlotsByName(this);

        recentsView_->setAttribute(Qt::WA_MacShowFocusRect, false);
        contactListView_->setAttribute(Qt::WA_MacShowFocusRect, false);

        connect(&Utils::InterConnector::instance(), SIGNAL(showNoContactsYet()), this, SLOT(showNoContactsYet()));
        connect(&Utils::InterConnector::instance(), SIGNAL(hideNoContactsYet()), this, SLOT(hideNoContactsYet()));

        connect(&Utils::InterConnector::instance(), SIGNAL(showNoRecentsYet()), this, SLOT(showNoRecentsYet()));
        connect(&Utils::InterConnector::instance(), SIGNAL(hideNoRecentsYet()), this, SLOT(hideNoRecentsYet()));

        if (_regim == Logic::MembersWidgetRegim::CONTACT_LIST)
        {
            connect(&Utils::InterConnector::instance(), SIGNAL(showNoSearchResults()), this, SLOT(showNoSearchResults()));
            connect(&Utils::InterConnector::instance(), SIGNAL(hideNoSearchResults()), this, SLOT(hideNoSearchResults()));

            connect(&Utils::InterConnector::instance(), SIGNAL(showSearchSpinner()), this, SLOT(showSearchSpinner()));
            connect(&Utils::InterConnector::instance(), SIGNAL(hideSearchSpinner()), this, SLOT(hideSearchSpinner()));

            connect(&Utils::InterConnector::instance(), SIGNAL(dialogClosed(QString)), this, SLOT(dialogClosed(QString)));
        }

		if (regim_ != Logic::MembersWidgetRegim::CONTACT_LIST)
		{
            buttonsFrame_->hide();
		}

		qobject_cast<QHBoxLayout*>(buttonsFrame_->layout())->insertWidget(1, recentsButton_);
        qobject_cast<QHBoxLayout*>(buttonsFrame_->layout())->insertWidget(3, livechatsButton_);

        Utils::grabTouchWidget(contactListView_->viewport(), true);
        Utils::grabTouchWidget(recentsView_->viewport(), true);
        Utils::grabTouchWidget(searchView_->viewport(), true);

        contactListView_->viewport()->grabGesture(Qt::TapAndHoldGesture);
        contactListView_->viewport()->installEventFilter(listEventFilter_);
        recentsView_->viewport()->grabGesture(Qt::TapAndHoldGesture);
        recentsView_->viewport()->installEventFilter(listEventFilter_);
        searchView_->viewport()->grabGesture(Qt::TapAndHoldGesture);
        searchView_->viewport()->installEventFilter(listEventFilter_);

        connect(QScroller::scroller(searchView_->viewport()), SIGNAL(stateChanged(QScroller::State)), this, SLOT(touchScrollStateChangedSearch(QScroller::State)), Qt::QueuedConnection);
        connect(QScroller::scroller(contactListView_->viewport()), SIGNAL(stateChanged(QScroller::State)), this, SLOT(touchScrollStateChangedCl(QScroller::State)), Qt::QueuedConnection);
        connect(QScroller::scroller(recentsView_->viewport()), SIGNAL(stateChanged(QScroller::State)), this, SLOT(touchScrollStateChangedRecents(QScroller::State)), Qt::QueuedConnection);


        if (Logic::is_delete_members_regim(regim_))
            contactListView_->setModel(chatMembersModel_);
        else
            if (regim_ != Logic::MembersWidgetRegim::SELECT_MEMBERS
            && regim_ != Logic::MembersWidgetRegim::SHARE_LINK
            && regim_ != Logic::MembersWidgetRegim::SHARE_TEXT)
            contactListView_->setModel(Logic::getContactListModel());

        connect(Logic::getContactListModel(), SIGNAL(select(QString, qint64)), this, SLOT(select(QString, qint64)), Qt::QueuedConnection);

        contactListView_->setItemDelegate(clDelegate_);
        connect(contactListView_, SIGNAL(clicked(const QModelIndex&)), this, SLOT(itemClicked(const QModelIndex&)), Qt::QueuedConnection);
        connect(contactListView_, SIGNAL(pressed(const QModelIndex&)), this, SLOT(itemPressed(const QModelIndex&)), Qt::QueuedConnection);
        connect(contactListView_, SIGNAL(clicked(const QModelIndex&)), this, SLOT(statsCLItemPressed(const QModelIndex&)), Qt::QueuedConnection);

        connect(contactListView_->verticalScrollBar(), SIGNAL(valueChanged(int)), Logic::getContactListModel(), SLOT(scrolled(int)), Qt::QueuedConnection);
        connect(contactListView_->verticalScrollBar(), SIGNAL(valueChanged(int)), Logic::getContactListModel(), SLOT(scrolled(int)), Qt::QueuedConnection);
        contactListView_->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
        connect(this, SIGNAL(groupClicked(int)), Logic::getContactListModel(), SLOT(groupClicked(int)), Qt::QueuedConnection);

        Logic::getUnknownsModel(); // just initialization
        recentsView_->setModel(Logic::getRecentsModel());
        recentsView_->setItemDelegate(recentsDelegate_);
        connect(recentsView_, SIGNAL(pressed(const QModelIndex&)), this, SLOT(itemPressed(const QModelIndex&)), Qt::QueuedConnection);
        connect(recentsView_, SIGNAL(clicked(const QModelIndex&)), this, SLOT(itemClicked(const QModelIndex&)), Qt::QueuedConnection);
        connect(recentsView_, SIGNAL(clicked(const QModelIndex&)), this, SLOT(statsRecentItemPressed(const QModelIndex&)), Qt::QueuedConnection);

        connect(contactListView_->verticalScrollBar(), SIGNAL(valueChanged(int)), Logic::getContactListModel(), SLOT(scrolled(int)), Qt::QueuedConnection);
        recentsView_->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);

        liveChatsView_->setModel(Logic::GetLiveChatsModel());
        liveChatsView_->setItemDelegate(liveChatsDelegate_);
        connect(liveChatsView_, SIGNAL(pressed(const QModelIndex&)), this, SLOT(liveChatsItemPressed(const QModelIndex&)), Qt::QueuedConnection);
        liveChatsView_->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);

        Utils::grabTouchWidget(liveChatsView_->viewport(), true);
        connect(QScroller::scroller(liveChatsView_->viewport()), SIGNAL(stateChanged(QScroller::State)), this, SLOT(touchScrollStateChangedLC(QScroller::State)), Qt::QueuedConnection);

        connect(Logic::getUnknownsModel(), SIGNAL(orderChanged()), this, SLOT(recentOrderChanged()), Qt::QueuedConnection);
        connect(Logic::getUnknownsModel(), SIGNAL(updated()), this, SLOT(recentOrderChanged()), Qt::QueuedConnection);
        connect(Logic::getUnknownsModel(), SIGNAL(updated()), recentsButton_, SLOT(update()), Qt::QueuedConnection);
        connect(Logic::getRecentsModel(), SIGNAL(orderChanged()), this, SLOT(recentOrderChanged()), Qt::QueuedConnection);
        connect(Logic::getRecentsModel(), SIGNAL(updated()), this, SLOT(recentOrderChanged()), Qt::QueuedConnection);
        connect(Logic::getRecentsModel(), SIGNAL(updated()), recentsButton_, SLOT(update()), Qt::QueuedConnection);
        connect(Logic::getContactListModel(), SIGNAL(contactChanged(QString)), recentsButton_, SLOT(update()), Qt::QueuedConnection);
        connect(Logic::getRecentsModel(), SIGNAL(updated()), livechatsButton_, SLOT(update()), Qt::QueuedConnection);
        connect(Logic::getContactListModel(), SIGNAL(contactChanged(QString)), livechatsButton_, SLOT(update()), Qt::QueuedConnection);
        connect(Logic::getContactListModel(), SIGNAL(switchTab(QString)), this, SLOT(switchTab(QString)), Qt::QueuedConnection);

        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::updated, this, [=]()
        {
            if (recentsView_ && recentsView_->model() == Logic::getRecentsModel())
                emit Logic::getRecentsModel()->refresh();
        });
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::updated, this, [=]()
        {
            if (recentsView_ && recentsView_->model() == Logic::getUnknownsModel())
                emit Logic::getUnknownsModel()->refresh();
        });

        searchView_->setModel(Logic::getCurrentSearchModel(regim_));

        if (regim_ == Logic::MembersWidgetRegim::CONTACT_LIST)
            searchItemDelegate_ = new Logic::RecentItemDelegate(this);
        else
            searchItemDelegate_ = new Logic::ContactListItemDelegate(this, _regim);

        if (_regim == Logic::MembersWidgetRegim::CONTACT_LIST)
            searchItemDelegate_->setRegim(Logic::MembersWidgetRegim::HISTORY_SEARCH);
        searchView_->setItemDelegate(searchItemDelegate_);

        connect(searchView_, SIGNAL(pressed(const QModelIndex&)), this, SLOT(itemPressed(const QModelIndex&)), Qt::QueuedConnection);
        connect(searchView_, SIGNAL(clicked(const QModelIndex&)), this, SLOT(itemClicked(const QModelIndex&)), Qt::QueuedConnection);
        connect(searchView_, SIGNAL(clicked(const QModelIndex&)), this, SLOT(statsSearchItemPressed(const QModelIndex&)), Qt::QueuedConnection);
        connect(searchView_, SIGNAL(activated(const QModelIndex&)), this, SLOT(searchClicked(const QModelIndex&)), Qt::QueuedConnection);
        connect(Logic::getCurrentSearchModel(regim_), SIGNAL(results()), this, SLOT(searchResultsFromModel()), Qt::QueuedConnection);
        connect(this, SIGNAL(searchEnd()), Logic::getCurrentSearchModel(regim_), SLOT(searchEnded()), Qt::QueuedConnection);

        // Prepare settings
        {
            settingsTab_ = new SettingsTab(contactListPage_);
            stackedWidget_->insertWidget(SETTINGS, settingsTab_);
        }

        connect(clTabButton_, SIGNAL(toggled(bool)), this, SLOT(allClicked()), Qt::QueuedConnection);
        connect(settingsTabButton_, SIGNAL(toggled(bool)), this, SLOT(settingsClicked()), Qt::QueuedConnection);
        connect(recentsButton_, SIGNAL(toggled(bool)), this, SLOT(recentsClicked()), Qt::QueuedConnection);
        connect(livechatsButton_, SIGNAL(toggled(bool)), this, SLOT(liveChatsClicked()), Qt::QueuedConnection);
        connect(Logic::getContactListModel(), SIGNAL(needSwitchToRecents()), this, SLOT(switchToRecents()));
        connect(Logic::getRecentsModel(), SIGNAL(selectContact(QString)), this, SLOT(select(QString)), Qt::DirectConnection);
        connect(Logic::getUnknownsModel(), SIGNAL(selectContact(QString)), this, SLOT(select(QString)), Qt::DirectConnection);
        clTabButton_->setFocusPolicy(Qt::NoFocus);
        clTabButton_->setCursor(Qt::PointingHandCursor);
        settingsTabButton_->setFocusPolicy(Qt::NoFocus);
        settingsTabButton_->setCursor(Qt::PointingHandCursor);
        recentsButton_->setCursor(Qt::PointingHandCursor);
        recentsButton_->setFocusPolicy(Qt::NoFocus);
        livechatsButton_->setCursor(Qt::PointingHandCursor);
        livechatsButton_->setFocusPolicy(Qt::NoFocus);
        Testing::setAccessibleName(recentsButton_, "RecentTabButton");

        if (regim_ == Logic::MembersWidgetRegim::CONTACT_LIST)
        {
            connect(get_gui_settings(), SIGNAL(received()), this, SLOT(guiSettingsChanged()), Qt::QueuedConnection);
            guiSettingsChanged();
        }

        scrollTimer_ = new QTimer(this);
        scrollTimer_->setInterval(autoscroll_timeout);
        scrollTimer_->setSingleShot(false);
        connect(scrollTimer_, SIGNAL(timeout()), this, SLOT(autoScroll()), Qt::QueuedConnection);

        connect(GetDispatcher(), &core_dispatcher::typingStatus, this, &ContactList::typingStatus);

        connect(GetDispatcher(), SIGNAL(messagesReceived(QString, QVector<QString>)), this, SLOT(messagesReceived(QString, QVector<QString>)));

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::unknownsGoSeeThem, this, [this]()
        {
            if (recentsView_->model() != Logic::getUnknownsModel())
            {
                recentsView_->setModel(Logic::getUnknownsModel());
                recentsView_->setItemDelegate(unknownsDelegate_);
                recentsView_->update();
                if (platform::is_apple())
                    emit Utils::InterConnector::instance().forceRefreshList(recentsView_->model(), true);
            }
        });
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::unknownsGoBack, this, [this]()
        {
            if (recentsView_->model() != Logic::getRecentsModel())
            {
                Logic::getUnknownsModel()->markAllRead();
                recentsView_->setModel(Logic::getRecentsModel());
                recentsView_->setItemDelegate(recentsDelegate_);
                recentsView_->update();
                if (platform::is_apple())
                    emit Utils::InterConnector::instance().forceRefreshList(recentsView_->model(), true);
            }
        });
    }

    ContactList::~ContactList()
    {

    }

    void ContactList::setSearchMode(bool _search)
    {
        if (isSearchMode() == _search)
            return;

        if (_search)
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_search);

        if (_search)
        {
            stackedWidget_->setCurrentIndex(SEARCH);
            Logic::getCurrentSearchModel(regim_)->setFocus();
        }
        else
        {
            stackedWidget_->setCurrentIndex(currentTab_);
        }
    }

    bool ContactList::isSearchMode() const
    {
        return stackedWidget_->currentIndex() == SEARCH;
    }

    bool ContactList::isContactListMode() const
    {
        return stackedWidget_->currentIndex() == ALL;
    }

    bool ContactList::shouldHideSearch() const
    {
        return currentTab_ == SETTINGS || currentTab_ == LIVE_CHATS;
    }

    QString ContactList::getAimid(const QModelIndex& _current) const
    {
        QString aimid;
        if (Logic::is_delete_members_regim(regim_))
        {
            auto cont = _current.data().value<Data::ChatMemberInfo*>();
            if (!cont)
                return "";
            aimid = cont->AimId_;
        }
        else if (qobject_cast<const Logic::UnknownsModel*>(_current.model()))
        {
            Data::DlgState dlg = _current.data().value<Data::DlgState>();
            aimid = dlg.AimId_;
        }
        else if (qobject_cast<const Logic::RecentsModel*>(_current.model()))
        {
            Data::DlgState dlg = _current.data().value<Data::DlgState>();
            aimid = Logic::getRecentsModel()->isServiceAimId(dlg.AimId_) ? QString() : dlg.AimId_;
        }
        else if (qobject_cast<const Logic::ContactListModel*>(_current.model()))
        {
            Data::Contact* cont = _current.data().value<Data::Contact*>();
            if (!cont)
                return "";
            aimid = cont->AimId_;
        }
        else if (qobject_cast<const Logic::SearchModelDLG*>(_current.model()))
        {
            auto dlg = _current.data().value<Data::DlgState>();
            aimid = dlg.AimId_;
        }
        else
        {
            aimid = "";
        }
        return aimid;
    }

    void ContactList::selectionChanged(const QModelIndex & _current)
    {
        QString aimid = getAimid(_current);

        if (aimid.isEmpty())
            return;

        // TODO : check group contact & aimid
        if (!aimid.isEmpty())
        {
            qint64 mess_id = -1;
            const Logic::SearchModelDLG* searchModel;
            if ((searchModel = qobject_cast<const Logic::SearchModelDLG*>(_current.model())))
            {
                auto dlg = _current.data().value<Data::DlgState>();
                if (!dlg.IsContact_)
                    mess_id = dlg.SearchedMsgId_;
            }
            select(aimid, mess_id);
        }

        emit itemClicked(aimid);
        searchView_->selectionModel()->clearCurrentIndex();
    }

    void ContactList::searchResults(const QModelIndex & _current, const QModelIndex &)
    {
        if (regim_ != Logic::MembersWidgetRegim::CONTACT_LIST)
            return;
        if (!_current.isValid())
        {
            emit searchEnd();
            return;
        }

        if (qobject_cast<const Logic::SearchModelDLG*>(_current.model()))
        {
            selectionChanged(_current);
            emit searchEnd();
            return;
        }

        Data::Contact* cont = _current.data().value<Data::Contact*>();
        if (!cont)
        {
            searchView_->clearSelection();
            searchView_->selectionModel()->clearCurrentIndex();
            return;
        }

        if (cont->GetType() != Data::GROUP)
            select(cont->AimId_);

        setSearchMode(false);
        searchView_->clearSelection();
        searchView_->selectionModel()->clearCurrentIndex();

        emit searchEnd();
    }

    void ContactList::searchResult()
    {
        QModelIndex i = searchView_->selectionModel()->currentIndex();
        QModelIndex temp;
        searchResults(i, temp);
    }

    void ContactList::itemClicked(const QModelIndex& _current)
    {
        if (qobject_cast<const Logic::ContactListModel*>(_current.model()))
        {
            const auto membersModel = qobject_cast<const Logic::ChatMembersModel*>(_current.model());
            if (!membersModel)
            {
                Data::Contact* cont = _current.data().value<Data::Contact*>();
                if (cont->GetType() == Data::GROUP)
                {
                    emit groupClicked(cont->GroupId_);
                    return;
                }
            }
        }

        if (!(QApplication::mouseButtons() & Qt::RightButton || tapAndHoldModifier()))
        {
            selectionChanged(_current);
        }
    }

    void ContactList::itemPressed(const QModelIndex& _current)
    {
        if (qobject_cast<const Logic::RecentsModel*>(_current.model()) && Logic::getRecentsModel()->isServiceItem(_current))
        {
            if (QApplication::mouseButtons() & Qt::LeftButton && Logic::getRecentsModel()->isUnknownsButton(_current))
            {
                emit Utils::InterConnector::instance().unknownsGoSeeThem();
            }
            else if (QApplication::mouseButtons() & Qt::LeftButton && Logic::getRecentsModel()->isFavoritesGroupButton(_current))
            {
                Logic::getRecentsModel()->toggleFavoritesVisible();
            }
            return;
        }

        if (qobject_cast<const Logic::UnknownsModel *>(_current.model()) && (QApplication::mouseButtons() & Qt::LeftButton))
        {
            const auto rect = recentsView_->visualRect(_current);
            const auto pos1 = recentsView_->mapFromGlobal(QCursor::pos());
            if (rect.contains(pos1))
            {
                QPoint pos(pos1.x(), pos1.y() - rect.y());
                if (Logic::getUnknownsModel()->isServiceItem(_current))
                {
                    if (unknownsDelegate_->isInDeleteAllFrame(pos))
                    {
                        emit Utils::InterConnector::instance().unknownsDeleteThemAll();
                        return;
                    }
                }
                else
                {
                    const auto aimId = getAimid(_current);
                    if (!aimId.isEmpty())
                    {
                        if (unknownsDelegate_->isInAddContactFrame(pos) && Logic::getUnknownsModel()->unreads(_current.row()) == 0)
                        {
                            emit Utils::InterConnector::instance().profileSettingsUnknownAdd(aimId);
                            return;
                        }
                        else if (unknownsDelegate_->isInRemoveContactFrame(pos))
                        {
                            emit Utils::InterConnector::instance().profileSettingsUnknownRemove(aimId);
                            return;
                        }
                    }
                }
            }
        }

        if (QApplication::mouseButtons() & Qt::RightButton || tapAndHoldModifier())
        {
            triggerTapAndHold(false);

            if (qobject_cast<const Logic::RecentsModel *>(_current.model())
                || qobject_cast<const Logic::UnknownsModel *>(_current.model()))
            {
                showRecentsPopup_menu(_current);
            }
            else if (qobject_cast<const Logic::SearchModelDLG *>(_current.model()))
            {
                auto dlg = _current.data().value<Data::DlgState>();
                if (dlg.IsContact_)
                {
                    Data::DlgState dlg = _current.data(Qt::DisplayRole).value<Data::DlgState>();
                    QString aimId = dlg.AimId_;
                    auto cont = Logic::getContactListModel()->getContactItem(aimId);
                    showContactsPopupMenu(cont->get_aimid(), cont->is_chat());
                }
            }
            else
            {
                auto cont = _current.data(Qt::DisplayRole).value<Data::Contact*>();
                if (cont)
                    showContactsPopupMenu(cont->AimId_, cont->Is_chat_);
            }
        }
    }

    void ContactList::liveChatsItemPressed(const QModelIndex& _current)
    {
        liveChatsView_->selectionModel()->blockSignals(true);
        liveChatsView_->selectionModel()->setCurrentIndex(_current, QItemSelectionModel::ClearAndSelect);
        liveChatsView_->selectionModel()->blockSignals(false);
        Logic::GetLiveChatsModel()->select(_current);
    }

    void ContactList::statsRecentItemPressed(const QModelIndex& /*_current*/)
    {
        assert(!isSearchMode());
        if (!isSearchMode() && regim_ == Logic::MembersWidgetRegim::CONTACT_LIST)
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::open_chat_recents);
    }

    void ContactList::statsCLItemPressed(const QModelIndex& /*_current*/)
    {
        assert(!isSearchMode());
        if (!isSearchMode() && regim_ == Logic::MembersWidgetRegim::CONTACT_LIST)
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::open_chat_cl);
    }

    void ContactList::statsSearchItemPressed(const QModelIndex& _current)
    {
        if (regim_ == Logic::MembersWidgetRegim::CONTACT_LIST)
        {
            if (currentTab_ == ALL)
            {
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::open_chat_search_cl);
            }
            else
            {
                if (qobject_cast<Logic::SearchModelDLG*>(Logic::getCurrentSearchModel(regim_)))
                {
                    auto dlg = _current.data().value<Data::DlgState>();
                    if (!dlg.IsContact_)
                    {
                        if (isSearchInDialog_)
                            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_search_dialog_open);
                        else
                            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_search_openchatmessage);
                        return;
                    }
                }
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::open_chat_search_recents);
            }
        }
    }

	void ContactList::searchClicked(const QModelIndex& _current)
	{
		searchResults(_current, QModelIndex());
	}

	void ContactList::changeTab(CurrentTab _currTab)
	{
        if (getPictureOnlyView() && _currTab != RECENTS)
            return;

        if (_currTab != RECENTS)
        {
            emit Utils::InterConnector::instance().unknownsGoBack();
        }

		if (currentTab_ != _currTab)
		{
            if (currentTab_ == SETTINGS)
            {
                settingsTab_->cleanSelection();
                Utils::InterConnector::instance().restoreSidebar();
            }
            else if (currentTab_ == LIVE_CHATS)
            {
                emit Utils::InterConnector::instance().popPagesToRoot();
            }

            currentTab_ = _currTab;
            updateTabState(regim_ == Logic::MembersWidgetRegim::CONTACT_LIST);
        }
        else
        {
            updateCheckedButtons();
        }

        if (_currTab == SETTINGS)
            settingsTab_->settingsProfileClicked();
        else if (currentTab_ != LIVE_CHATS)
        {
            if (recentsView_->model() == Logic::getRecentsModel())
                Logic::getRecentsModel()->sendLastRead();
            else
                Logic::getUnknownsModel()->sendLastRead();
        }
    }

    void ContactList::triggerTapAndHold(bool _value)
    {
        tapAndHold_ = _value;
    }

    bool ContactList::tapAndHoldModifier() const
    {
        return tapAndHold_;
    }

    void ContactList::dragPositionUpdate(const QPoint& _pos, bool fromScroll)
    {
        int autoscroll_offset = Utils::scale_value(autoscroll_offset_cl);
        auto valid = true;
        if (isSearchMode())
        {
            QModelIndex index = QModelIndex();
            if (!_pos.isNull())
                index = searchView_->indexAt(_pos);

            if (index.isValid())
            {
                auto dlg = Logic::getSearchModelDLG()->data(index, Qt::DisplayRole).value<Data::DlgState>();
                auto role = Logic::getContactListModel()->getYourRole(dlg.AimId_);
                if (role == "notamember" || role == "readonly")
                    valid = false;
            }

            if (valid)
            {
                clDelegate_->setDragIndex(index);
                if (index.isValid())
                    emit Logic::getSearchModelDLG()->dataChanged(index, index);
            }
            else
            {
                clDelegate_->setDragIndex(QModelIndex());
            }

            scrolledView_ = searchView_;
        }
        else if (currentTab_ == RECENTS)
        {
            QModelIndex index = QModelIndex();
            if (!_pos.isNull())
                index = recentsView_->indexAt(_pos);

            if (index.isValid())
            {
                Data::DlgState dlg;
                if (recentsView_->model() == Logic::getRecentsModel())
                    dlg = Logic::getRecentsModel()->data(index, Qt::DisplayRole).value<Data::DlgState>();
                else
                    dlg = Logic::getUnknownsModel()->data(index, Qt::DisplayRole).value<Data::DlgState>();

                auto role = Logic::getContactListModel()->getYourRole(dlg.AimId_);
                if (role == "notamember" || role == "readonly")
                    valid = false;
            }

            if (valid)
            {
                if (recentsView_->itemDelegate() == recentsDelegate_)
                    recentsDelegate_->setDragIndex(index);
                else
                    unknownsDelegate_->setDragIndex(index);
                if (index.isValid())
                {
                    if (recentsView_->model() == Logic::getRecentsModel())
                        emit Logic::getRecentsModel()->dataChanged(index, index);
                    else
                        emit Logic::getUnknownsModel()->dataChanged(index, index);
                }
            }
            else
            {
                recentsDelegate_->setDragIndex(QModelIndex());
            }

            scrolledView_ = recentsView_;
            autoscroll_offset = Utils::scale_value(autoscroll_offset_recents);
        }
        else if (currentTab_ == ALL)
        {
            QModelIndex index = QModelIndex();
            if (!_pos.isNull())
                index = contactListView_->indexAt(_pos);

            if (index.isValid())
            {
                auto cont = Logic::getContactListModel()->data(index, Qt::DisplayRole).value<Data::Contact*>();
                auto role = Logic::getContactListModel()->getYourRole(cont->AimId_);
                if (role == "notamember" || role == "readonly")
                    valid = false;
            }

            if (valid)
            {
                clDelegate_->setDragIndex(index);
                if (index.isValid())
                    emit Logic::getContactListModel()->dataChanged(index, index);
            }
            else
            {
                clDelegate_->setDragIndex(QModelIndex());
            }

            scrolledView_ = contactListView_;
        }


        auto rTop = scrolledView_->rect();
        rTop.setBottomLeft(QPoint(rTop.x(), autoscroll_offset));

        auto rBottom = scrolledView_->rect();
        rBottom.setTopLeft(QPoint(rBottom.x(), rBottom.height() - autoscroll_offset));

        if (!_pos.isNull() && (rTop.contains(_pos) || rBottom.contains(_pos)))
        {
            scrollMultipler_ =  (rTop.contains(_pos)) ? 1 : -1;
            scrollTimer_->start();
        }
        else
        {
            scrollTimer_->stop();
        }

        if (!fromScroll)
            lastDragPos_ = _pos;

        scrolledView_->update();
    }

    void ContactList::dropFiles(const QPoint& _pos, const QList<QUrl> _files)
    {
        auto send = [](const QList<QUrl> files, const QString& aimId)
        {
            for (QUrl url : files)
            {
                if (url.isLocalFile())
                {
                    QFileInfo info(url.toLocalFile());
                    bool canDrop = !(info.isBundle() || info.isDir());
                    if (info.size() == 0)
                        canDrop = false;

                    if (canDrop)
                    {
                        Ui::GetDispatcher()->uploadSharedFile(aimId, url.toLocalFile());
                        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_dnd_recents);
                    }
                }
                else if (url.isValid())
                {
                    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                    collection.set_value_as_qstring("contact", aimId);
                    QString text = url.toString();
                    collection.set_value_as_string("message", text.toUtf8().data(), text.toUtf8().size());
                    Ui::GetDispatcher()->post_message_to_core("send_message", collection.get());
                }
            }
        };
        if (isSearchMode())
        {
            QModelIndex index = QModelIndex();
            if (!_pos.isNull())
            {
                index = searchView_->indexAt(_pos);
                Data::Contact* data  = qvariant_cast<Data::Contact*>(Logic::getSearchModelDLG()->data(index, Qt::DisplayRole));
                if (data)
                {
                    auto role = Logic::getContactListModel()->getYourRole(data->AimId_);
                    if (role != "notamember" && role != "readonly")
                    {
                        if (data->AimId_ != Logic::getContactListModel()->selectedContact())
                            Logic::getContactListModel()->select(data->AimId_, -1);

                        send(_files, data->AimId_);
                    }
                }
                emit Logic::getSearchModelDLG()->dataChanged(index, index);
            }
            clDelegate_->setDragIndex(QModelIndex());
        }
        else if (currentTab_ == RECENTS)
        {
            QModelIndex index = QModelIndex();
            if (!_pos.isNull())
            {
                index = recentsView_->indexAt(_pos);
                bool isRecents = true;
                Data::DlgState data;
                if (recentsView_->model() == Logic::getRecentsModel())
                {
                    data = qvariant_cast<Data::DlgState>(Logic::getRecentsModel()->data(index, Qt::DisplayRole));
                }
                else
                {
                    data = qvariant_cast<Data::DlgState>(Logic::getUnknownsModel()->data(index, Qt::DisplayRole));
                    isRecents = false;
                }
                if (!data.AimId_.isEmpty())
                {
                    auto role = Logic::getContactListModel()->getYourRole(data.AimId_);
                    if (role != "notamember" && role != "readonly")
                    {
                        if (data.AimId_ != Logic::getContactListModel()->selectedContact())
                            Logic::getContactListModel()->select(data.AimId_, -1);

                        send(_files, data.AimId_);
                        if (isRecents)
                            emit Logic::getRecentsModel()->dataChanged(index, index);
                        else
                            emit Logic::getUnknownsModel()->dataChanged(index, index);
                    }
                }
            }
            recentsDelegate_->setDragIndex(QModelIndex());
            unknownsDelegate_->setDragIndex(QModelIndex());
        }
        else if (currentTab_  == ALL)
        {
            QModelIndex index = QModelIndex();
            if (!_pos.isNull())
            {
                index = contactListView_->indexAt(_pos);
                Data::Contact* data  = qvariant_cast<Data::Contact*>(Logic::getContactListModel()->data(index, Qt::DisplayRole));
                if (data)
                {
                    auto role = Logic::getContactListModel()->getYourRole(data->AimId_);
                    if (role != "notamember" && role != "readonly")
                    {
                        if (data->AimId_ != Logic::getContactListModel()->selectedContact())
                            Logic::getContactListModel()->select(data->AimId_, -1);

                        send(_files, data->AimId_);
                    }
                }
                emit Logic::getContactListModel()->dataChanged(index, index);
            }
            clDelegate_->setDragIndex(QModelIndex());
        }
    }

    void ContactList::showContactList()
    {
        changeTab(ALL);
    }

    void ContactList::recentsClicked()
    {
        if (currentTab_ == RECENTS)
            emit Utils::InterConnector::instance().activateNextUnread();
        else
            changeTab(RECENTS);
    }

    void ContactList::liveChatsClicked()
    {
        changeTab(LIVE_CHATS);
        Logic::GetLiveChatsModel()->initIfNeeded();
        emit Utils::InterConnector::instance().liveChatsShow();
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::livechats_page_open);
    }

    void ContactList::switchToRecents()
    {
        changeTab(RECENTS);
    }

    void ContactList::allClicked()
    {
        changeTab(ALL);
    }

    void ContactList::settingsClicked()
    {
        changeTab(SETTINGS);
    }

    void ContactList::updateTabState(bool _save)
    {
        if (_save)
        {
            //get_gui_settings()->set_value<int>(settings_current_cl_tab, CurrentTab_);
        }

        stackedWidget_->setCurrentIndex(currentTab_);
        updateCheckedButtons();

        if (regim_ == Logic::MembersWidgetRegim::CONTACT_LIST)
            emit Utils::InterConnector::instance().makeSearchWidgetVisible(currentTab_ != SETTINGS && currentTab_ != LIVE_CHATS);

        recentOrderChanged();
    }

    void ContactList::updateCheckedButtons()
    {
        clTabButton_->blockSignals(true);
        settingsTabButton_->blockSignals(true);
        recentsButton_->blockSignals(true);
        livechatsButton_->blockSignals(true);

        clTabButton_->setChecked(currentTab_ == ALL);
        settingsTabButton_->setChecked(currentTab_ == SETTINGS);
        recentsButton_->setChecked(currentTab_ == RECENTS);
        livechatsButton_->setChecked(currentTab_ == LIVE_CHATS);

        clTabButton_->blockSignals(false);
        settingsTabButton_->blockSignals(false);
        recentsButton_->blockSignals(false);
        livechatsButton_->blockSignals(false);
    }

    void ContactList::guiSettingsChanged()
    {
        currentTab_ = 0;//get_gui_settings()->get_value<int>(settings_current_cl_tab, 0);
        updateTabState(false);
    }

    void ContactList::searchUpOrDownPressed(bool _isUpPressed)
    {
        auto inc = _isUpPressed ? -1 : 1;

        QModelIndex i = searchView_->selectionModel()->currentIndex();
        i = Logic::getCurrentSearchModel(regim_)->index(i.row() + inc);
        if (!i.isValid())
            return;

        searchView_->selectionModel()->blockSignals(true);
        searchView_->selectionModel()->setCurrentIndex(i, QItemSelectionModel::ClearAndSelect);
        searchView_->selectionModel()->blockSignals(false);
        Logic::getCurrentSearchModel(regim_)->emitChanged(i.row() - inc, i.row());
        searchView_->scrollTo(i);
    }

    void ContactList::searchUpPressed()
    {
        searchUpOrDownPressed(true);
    }

    void ContactList::searchDownPressed()
    {
        searchUpOrDownPressed(false);
	}

	void ContactList::onSendMessage(QString _contact)
	{
		switchTab(_contact);
	}

	void ContactList::recentOrderChanged()
	{
        if (currentTab_ == RECENTS)
        {
            recentsView_->selectionModel()->blockSignals(true);
            if (recentsView_->model() == Logic::getRecentsModel())
                recentsView_->selectionModel()->setCurrentIndex(Logic::getRecentsModel()->contactIndex(Logic::getContactListModel()->selectedContact()), QItemSelectionModel::ClearAndSelect);
            else
                recentsView_->selectionModel()->setCurrentIndex(Logic::getUnknownsModel()->contactIndex(Logic::getContactListModel()->selectedContact()), QItemSelectionModel::ClearAndSelect);
            recentsView_->selectionModel()->blockSignals(false);
        }
    }

    void ContactList::touchScrollStateChangedRecents(QScroller::State _state)
    {
        recentsView_->blockSignals(_state != QScroller::Inactive);
        recentsView_->selectionModel()->blockSignals(_state != QScroller::Inactive);
        if (recentsView_->model() == Logic::getRecentsModel())
        {
            recentsView_->selectionModel()->setCurrentIndex(Logic::getRecentsModel()->contactIndex(Logic::getContactListModel()->selectedContact()), QItemSelectionModel::ClearAndSelect);
            recentsDelegate_->blockState(_state != QScroller::Inactive);
        }
        else
        {
            recentsView_->selectionModel()->setCurrentIndex(Logic::getUnknownsModel()->contactIndex(Logic::getContactListModel()->selectedContact()), QItemSelectionModel::ClearAndSelect);
            unknownsDelegate_->blockState(_state != QScroller::Inactive);
        }
    }

    void ContactList::touchScrollStateChangedCl(QScroller::State _state)
    {
        contactListView_->blockSignals(_state != QScroller::Inactive);
        contactListView_->selectionModel()->blockSignals(_state != QScroller::Inactive);
        contactListView_->selectionModel()->setCurrentIndex(Logic::getContactListModel()->contactIndex(Logic::getContactListModel()->selectedContact()), QItemSelectionModel::ClearAndSelect);
        clDelegate_->blockState(_state != QScroller::Inactive);
    }

    void ContactList::touchScrollStateChangedSearch(QScroller::State _state)
    {
        searchView_->blockSignals(_state != QScroller::Inactive);
        searchView_->selectionModel()->blockSignals(_state != QScroller::Inactive);
        clDelegate_->blockState(_state != QScroller::Inactive);
    }

    void ContactList::touchScrollStateChangedLC(QScroller::State _state)
    {
        liveChatsView_->blockSignals(_state != QScroller::Inactive);
        liveChatsView_->selectionModel()->blockSignals(_state != QScroller::Inactive);
        liveChatsView_->selectionModel()->setCurrentIndex(Logic::getContactListModel()->contactIndex(Logic::getContactListModel()->selectedContact()), QItemSelectionModel::ClearAndSelect);
        liveChatsDelegate_->blockState(_state != QScroller::Inactive);
    }

    void ContactList::changeSelected(QString _aimId, bool _isRecent)
    {
        QListView* current_view = NULL;
        QModelIndex clIndex;

        if (_isRecent)
        {
            current_view = recentsView_;
            if (current_view->selectionModel())
                current_view->selectionModel()->blockSignals(true);
            clIndex = Logic::getRecentsModel()->contactIndex(_aimId);
            if (!clIndex.isValid())
                clIndex = Logic::getUnknownsModel()->contactIndex(_aimId);
        }
        else
        {
            current_view = contactListView_;
            if (current_view->selectionModel())
                current_view->selectionModel()->blockSignals(true);
            clIndex = Logic::getContactListModel()->contactIndex(_aimId);
        }

        if (clIndex.isValid())
        {
            if (current_view->selectionModel())
                current_view->selectionModel()->setCurrentIndex(clIndex, QItemSelectionModel::ClearAndSelect);
            current_view->scrollTo(clIndex);
        }
        else
        {
            if (current_view->selectionModel())
                current_view->selectionModel()->clearSelection();
        }
        current_view->update();
        if (current_view->selectionModel())
            current_view->selectionModel()->blockSignals(false);
    }

    void ContactList::select(QString _aimId, qint64 _message_id)
    {
        const auto isSameContact = Logic::getContactListModel()->selectedContact() == _aimId;

        if (regim_ == Logic::CONTACT_LIST)
            Logic::getContactListModel()->setCurrent(_aimId, _message_id);

        changeSelected(_aimId, true);
        changeSelected(_aimId, false);

        emit itemSelected(_aimId, _message_id);

        if (isSearchMode() && regim_ == Logic::CONTACT_LIST)
        {
            emit searchEnd();
        }

        if (currentTab_ == SETTINGS)
            recentsClicked();

        if (isSameContact)
            Logic::getRecentsModel()->sendLastRead(_aimId);
    }

    void ContactList::searchResultsFromModel()
    {
        auto pat = Logic::getCurrentSearchModel(regim_)->getCurrentPattern();
        if (pat == lastSearchPattern_)
            return;

        lastSearchPattern_ = pat;
        auto init_ind = 0;
        auto model = qobject_cast<Logic::SearchModelDLG*>(Logic::getCurrentSearchModel(regim_));
        if (model)
        {
            init_ind = model->get_abs_index(init_ind);
        }

        QModelIndex i = Logic::getCurrentSearchModel(regim_)->index(init_ind);
        if (!i.isValid())
            return;

        searchView_->selectionModel()->blockSignals(true);
        searchView_->selectionModel()->setCurrentIndex(i, QItemSelectionModel::ClearAndSelect);
        searchView_->selectionModel()->blockSignals(false);
        Logic::getCurrentSearchModel(regim_)->emitChanged(i.row() - 1, i.row());
        searchView_->scrollTo(i);
    }

    void ContactList::showContactsPopupMenu(QString aimId, bool _is_chat)
    {
        if (!popupMenu_)
        {
            popupMenu_ = new ContextMenu(this);
            connect(popupMenu_, SIGNAL(triggered(QAction*)), this, SLOT(showPopupMenu(QAction*)));
        }
        else
        {
            popupMenu_->clear();
        }

        if (Logic::is_delete_members_regim(regim_))
            return;

        if (!_is_chat)
        {
#ifndef STRIP_VOIP
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_callmenu_100.png")), QT_TRANSLATE_NOOP("context_menu","Call"), makeData("contacts/call", aimId));
#endif //STRIP_VOIP
        }
        popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_profile_100.png")), QT_TRANSLATE_NOOP("context_menu", "Profile"), makeData("contacts/Profile", aimId));
        popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_ignore_100.png")), QT_TRANSLATE_NOOP("context_menu", "Ignore"), makeData("contacts/ignore", aimId));
        if (!_is_chat)
        {
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_spam_100.png")), QT_TRANSLATE_NOOP("context_menu", "Report spam"), makeData("contacts/spam", aimId));
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_delete_100.png")), QT_TRANSLATE_NOOP("context_menu", "Delete"), makeData("contacts/remove", aimId));
        }
        else
        {
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_delete_100.png")), QT_TRANSLATE_NOOP("context_menu", "Leave and delete"), makeData("contacts/remove", aimId));
        }

        popupMenu_->popup(QCursor::pos());
    }

    void ContactList::showRecentsPopup_menu(const QModelIndex& _current)
    {
        if (recentsView_->model() == Logic::getUnknownsModel())
        {
            return;
        }

        if (!popupMenu_)
        {
            popupMenu_ = new ContextMenu(this);
            Testing::setAccessibleName(popupMenu_, "popup_menu_");
            connect(popupMenu_, SIGNAL(triggered(QAction*)), this, SLOT(showPopupMenu(QAction*)));
        }
        else
        {
            popupMenu_->clear();
        }

        Data::DlgState dlg = _current.data(Qt::DisplayRole).value<Data::DlgState>();
        QString aimId = dlg.AimId_;

        if (dlg.UnreadCount_ != 0)
        {
            auto icon = QIcon(Utils::parse_image_name(":/resources/dialog_markread_100.png"));
            popupMenu_->addActionWithIcon(icon, QT_TRANSLATE_NOOP("context_menu", "Mark as read"), makeData("recents/mark_read", aimId));
            //            Testing::setAccessibleName(icon, "recents/mark_read");
        }
        if (Logic::getContactListModel()->isMuted(dlg.AimId_))
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_unmute_100.png")), QT_TRANSLATE_NOOP("context_menu", "Turn on notifications"), makeData("recents/unmute", aimId));
        else
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_mute_100.png")), QT_TRANSLATE_NOOP("context_menu", "Turn off notifications"), makeData("recents/mute", aimId));

        auto ignore_icon = QIcon(Utils::parse_image_name(":/resources/dialog_ignore_100.png"));
        //      Testing::setAccessibleName(ignore_icon, "recents/ignore_icon");
        /*auto ignore_action = */popupMenu_->addActionWithIcon(ignore_icon, QT_TRANSLATE_NOOP("context_menu", "Ignore"), makeData("recents/ignore", aimId));
        //    Testing::setAccessibleName(ignore_action, "recents/ignore_action");


        //auto item = Logic::getContactListModel()->getContactItem(aimId);
        //bool is_group = (item && item->is_chat());

        if (Logic::getRecentsModel()->isFavorite(aimId))
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_unfavorite_100.png")), QT_TRANSLATE_NOOP("context_menu", "Remove from favorites"), makeData("recents/unfavorite", aimId));
        else
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_closechat_100.png")), QT_TRANSLATE_NOOP("context_menu", "Close"), makeData("recents/close", aimId));

        if (Logic::getRecentsModel()->totalUnreads() != 0)
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_markread_all_100.png")), QT_TRANSLATE_NOOP("context_menu", "Mark all read"), makeData("recents/read_all"));

        popupMenu_->popup(QCursor::pos());
    }

    void ContactList::showPopupMenu(QAction* _action)
    {
        auto params = _action->data().toMap();
        const QString command = params["command"].toString();
        const QString aimId = params["contact"].toString();

        if (command == "recents/mark_read")
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::recents_read);
            Logic::getRecentsModel()->sendLastRead(aimId);
        }
        else if (command == "recents/mute")
        {
            Logic::getRecentsModel()->muteChat(aimId, true);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::mute_recents_menu);
        }
        else if (command == "recents/unmute")
        {
            Logic::getRecentsModel()->muteChat(aimId, false);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::unmute);
        }
        else if (command == "recents/ignore" || command == "contacts/ignore")
        {
            if (Logic::getContactListModel()->ignoreContactWithConfirm(aimId))
                GetDispatcher()->post_stats_to_core(command == "recents/ignore"
                                                    ? core::stats::stats_event_names::ignore_recents_menu : core::stats::stats_event_names::ignore_cl_menu);
        }
        else if (command == "recents/close")
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::recents_close);
            Logic::getRecentsModel()->hideChat(aimId);
        }
        else if (command == "recents/read_all")
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::recents_readall);
            Logic::getRecentsModel()->markAllRead();
        }
        else if (command == "contacts/call")
        {
            Ui::GetDispatcher()->getVoipController().setStartV(aimId.toUtf8(), false);
        }
        else if (command == "contacts/Profile")
        {
            emit Utils::InterConnector::instance().profileSettingsShow(aimId);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_cl);
        }
        else if (command == "contacts/spam")
        {
            if (Logic::getContactListModel()->blockAndSpamContact(aimId))
            {
                Logic::getContactListModel()->removeContactFromCL(aimId);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::spam_cl_menu);
            }
        }
        else if (command == "contacts/remove")
        {
            QString text = QT_TRANSLATE_NOOP("popup_window", Logic::getContactListModel()->isChat(aimId)
                                             ? "Are you sure you want to leave chat?" : "Are you sure you want to delete contact?");

            auto confirm = Utils::GetConfirmationWithTwoButtons(
                                                                QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                                                                QT_TRANSLATE_NOOP("popup_window", "Yes"),
                                                                text,
                                                                Logic::getContactListModel()->getDisplayName(aimId),
                                                                NULL
                                                                );

            if (confirm)
            {
                Logic::getContactListModel()->removeContactFromCL(aimId);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::delete_cl_menu);
            }
        }
        else if (command == "recents/unfavorite")
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("contact", aimId);
            Ui::GetDispatcher()->post_message_to_core("unfavorite", collection.get());
        }
    }

    void ContactList::switchTab(QString aimId)
    {
        changeTab(RECENTS);
    }

    void ContactList::autoScroll()
    {
        if (scrolledView_)
        {
            scrolledView_->verticalScrollBar()->setValue(scrolledView_->verticalScrollBar()->value() - (Utils::scale_value(autoscroll_speed_pixels) * scrollMultipler_));
            dragPositionUpdate(lastDragPos_, true);
        }
    }

    void ContactList::showNoContactsYet(QWidget *_parent, QWidget *_list, QLayout *_layout)
    {
        if (noContactsYetShown_)
            return;
        if (!noContactsYet_)
        {
            noContactsYetShown_ = true;
            _list->setMaximumHeight(Utils::scale_value(50));
            noContactsYet_ = new QWidget(_parent);
            noContactsYet_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            _layout->addWidget(noContactsYet_);
            {
                auto mainLayout = new QVBoxLayout(noContactsYet_);
                mainLayout->setAlignment(Qt::AlignCenter);
                mainLayout->setContentsMargins(Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0));
                mainLayout->setSpacing(Utils::scale_value(0));
                {
                    auto noContactsWidget = new QWidget(noContactsYet_);
                    auto noContactsLayout = new QVBoxLayout(noContactsWidget);
                    noContactsWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
                    noContactsLayout->setAlignment(Qt::AlignCenter);
                    noContactsLayout->setContentsMargins(Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0));
                    noContactsLayout->setSpacing(Utils::scale_value(20));
                    {
                        auto noContactsPlaceholder = new QWidget(noContactsWidget);
                        noContactsPlaceholder->setObjectName("noContacts");
                        noContactsPlaceholder->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                        noContactsPlaceholder->setFixedHeight(Utils::scale_value(160));
                        noContactsLayout->addWidget(noContactsPlaceholder);
                    }
                    {
                        auto noContactLabel = new QLabel(noContactsWidget);
                        noContactLabel->setObjectName("noContactsLabel");
                        noContactLabel->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                        noContactLabel->setAlignment(Qt::AlignCenter);
                        noContactLabel->setWordWrap(true);
                        noContactLabel->setText(QT_TRANSLATE_NOOP("placeholders", "Looks like you have no contacts yet"));
                        noContactsLayout->addWidget(noContactLabel);
                    }
                    mainLayout->addWidget(noContactsWidget);
                }
            }

            emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_FindFriend);
        }
    }

    void ContactList::hideNoContactsYet(QWidget *_list, QLayout *_layout)
    {
        if (noContactsYet_)
        {
            noContactsYet_->setHidden(true);
            _layout->removeWidget(noContactsYet_);
            noContactsYet_ = nullptr;
            _list->setMaximumHeight(QWIDGETSIZE_MAX);

            emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_HideFindFriend);

            noContactsYetShown_ = false;
        }
    }

    void ContactList::showNoRecentsYet(QWidget *_parent, QWidget *_list, QLayout *_layout, std::function<void()> _action)
    {
        if (noRecentsYetShown_)
            return;
        if (!noRecentsYet_)
        {
            noRecentsYetShown_ = true;
            _list->setHidden(true);
            noRecentsYet_ = new QWidget(_parent);
            noRecentsYet_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            _layout->addWidget(noRecentsYet_);
            {
                auto mainLayout = new QVBoxLayout(noRecentsYet_);
                mainLayout->setAlignment(Qt::AlignCenter);
                mainLayout->setContentsMargins(Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0));
                mainLayout->setSpacing(Utils::scale_value(0));
                {
                    auto noRecentsWidget = new QWidget(noRecentsYet_);
                    auto noRecentsLayout = new QVBoxLayout(noRecentsWidget);
                    noRecentsWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
                    noRecentsLayout->setAlignment(Qt::AlignCenter);
                    noRecentsLayout->setContentsMargins(Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0));
                    noRecentsLayout->setSpacing(Utils::scale_value(20));
                    {
                        auto noRecentsPlaceholder = new QWidget(noRecentsWidget);
                        noRecentsPlaceholder->setObjectName("noRecents");
                        noRecentsPlaceholder->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                        noRecentsPlaceholder->setFixedHeight(Utils::scale_value(160));
                        noRecentsLayout->addWidget(noRecentsPlaceholder);
                    }
                    {
                        auto noRecentsLabel = new QLabel(noRecentsWidget);
                        noRecentsLabel->setObjectName("noRecentsLabel");
                        noRecentsLabel->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                        noRecentsLabel->setAlignment(Qt::AlignCenter);
                        noRecentsLabel->setText(QT_TRANSLATE_NOOP("placeholders", "You have no opened chats yet"));
                        noRecentsLayout->addWidget(noRecentsLabel);
                    }
                    {
                        auto noRecentsButtonWidget = new QWidget(noRecentsWidget);
                        auto noRecentsButtonLayout = new QHBoxLayout(noRecentsButtonWidget);
                        noRecentsButtonWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
                        noRecentsButtonWidget->setStyleSheet("background-color: transparent;");
                        noRecentsButtonLayout->setContentsMargins(0, 0, 0, 0);
                        noRecentsButtonLayout->setAlignment(Qt::AlignCenter);
                        {
                            auto noRecentsButton = new QPushButton(noRecentsButtonWidget);
                            Utils::ApplyStyle(noRecentsButton, Ui::CommonStyle::getGreenButtonStyle());
                            noRecentsButton->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
                            noRecentsButton->setFlat(true);
                            noRecentsButton->setCursor(Qt::PointingHandCursor);
                            noRecentsButton->setText(QT_TRANSLATE_NOOP("placeholders", "Write a message"));
                            _parent->connect(noRecentsButton, &QPushButton::clicked, _action);
                            noRecentsButtonLayout->addWidget(noRecentsButton);
                        }
                        noRecentsLayout->addWidget(noRecentsButtonWidget);
                    }
                    mainLayout->addWidget(noRecentsWidget);
                }
            }
        }
    }

    void ContactList::hideNoRecentsYet(QWidget *_list, QLayout *_layout)
    {
        if (noRecentsYet_)
        {
            noRecentsYet_->setHidden(true);
            _layout->removeWidget(noContactsYet_);
            noRecentsYet_ = nullptr;
            _list->setHidden(false);
            noRecentsYetShown_ = false;
        }
    }

    void ContactList::showNoSearchResults(QWidget *_parent, QWidget *_list, QLayout *_layout)
    {
        _list->setHidden(true);

        if (noSearchResultsShown_)
            return;

        noSearchResultsShown_ = true;
        if (!noSearchResults_)
        {
            _list->setHidden(true);
            noSearchResults_ = new QWidget(_parent);
            noSearchResults_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            _layout->addWidget(noSearchResults_);
            {
                auto mainLayout = new QVBoxLayout(noSearchResults_);
                mainLayout->setAlignment(Qt::AlignCenter);
                mainLayout->setContentsMargins(Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0));
                mainLayout->setSpacing(Utils::scale_value(0));
                {
                    auto noSearchResultsWidget = new QWidget(noSearchResults_);
                    auto noSearchLayout = new QVBoxLayout(noSearchResultsWidget);
                    noSearchResultsWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
                    noSearchLayout->setAlignment(Qt::AlignCenter);
                    noSearchLayout->setContentsMargins(Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0));
                    noSearchLayout->setSpacing(Utils::scale_value(20));
                    {
                        auto noSearchResultsPlaceholder = new QWidget(noSearchResultsWidget);
                        noSearchResultsPlaceholder->setObjectName("noSearchResults");
                        noSearchResultsPlaceholder->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                        noSearchResultsPlaceholder->setFixedHeight(Utils::scale_value(160));
                        noSearchLayout->addWidget(noSearchResultsPlaceholder);
                    }
                    {
                        auto noSearchResultsLabel = new QLabel(noSearchResultsWidget);
                        noSearchResultsLabel->setObjectName("noSearchResultsLabel");
                        noSearchResultsLabel->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                        noSearchResultsLabel->setAlignment(Qt::AlignCenter);
                        noSearchResultsLabel->setText(QT_TRANSLATE_NOOP("placeholders", "No messages found"));
                        noSearchLayout->addWidget(noSearchResultsLabel);
                    }

                    mainLayout->addWidget(noSearchResultsWidget);
                }
            }
        }
        else
        {
            _list->setHidden(true);
            noSearchResults_->setHidden(false);
        }
    }

    void ContactList::hideNoSearchResults(QWidget *_list, QLayout *_layout)
    {
        if (noSearchResultsShown_)
        {
            noSearchResults_->setHidden(true);
            _list->setHidden(false);
            noSearchResultsShown_ = false;
        }
    }

    void ContactList::showSearchSpinner(QWidget *_parent, QWidget *_list, QLayout *_layout)
    {
        _list->setHidden(true);

        if (searchSpinnerShown_)
            return;

        if (!searchSpinner_)
        {
            searchSpinnerShown_ = true;
            _list->setHidden(true);
            searchSpinner_ = new QWidget(_parent);
            searchSpinner_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            _layout->addWidget(searchSpinner_);
            {
                auto mainLayout = new QVBoxLayout(searchSpinner_);
                mainLayout->setAlignment(Qt::AlignCenter);
                mainLayout->setContentsMargins(Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0));
                mainLayout->setSpacing(Utils::scale_value(0));
                {
                    auto searchSpinnerWidget = new QWidget(searchSpinner_);
                    auto searchSpinnerLayout = new QVBoxLayout(searchSpinnerWidget);
                    searchSpinnerWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
                    searchSpinnerLayout->setAlignment(Qt::AlignCenter);
                    searchSpinnerLayout->setContentsMargins(Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0));
                    searchSpinnerLayout->setSpacing(Utils::scale_value(20));
                    {
                        auto spinner = new QMovie(":/resources/gifs/r_spiner200.gif");
                        spinner->setScaledSize(QSize(Utils::scale_value(40), Utils::scale_value(40)));

                        auto w = new QLabel(searchSpinnerWidget);
                        w->setMovie(spinner);
                        searchSpinnerLayout->addWidget(w);
                        spinner->start();
                    }

                    mainLayout->addWidget(searchSpinnerWidget);
                }
            }
        }
    }

    void ContactList::hideSearchSpinner(QWidget *_list, QLayout *_layout)
    {
        if (searchSpinnerShown_)
        {
            searchSpinner_->setHidden(true);
            _layout->removeWidget(searchSpinner_);
            searchSpinner_ = nullptr;
            _list->setHidden(false);
            searchSpinnerShown_ = false;
        }
    }

    void ContactList::hideNoContactsYet()
    {
        hideNoContactsYet(contactListView_, contactListLayout_);
    }

    void ContactList::hideNoRecentsYet()
    {
        hideNoRecentsYet(recentsView_, recentsLayout_);
    }

    void ContactList::hideSearchSpinner()
    {
        hideSearchSpinner(searchView_, searchLayout_);
    }

    void ContactList::showSearchSpinner()
    {
        showSearchSpinner(searchPage_, searchView_, searchLayout_);
    }

    void ContactList::hideNoSearchResults()
    {
        hideNoSearchResults(searchView_, searchLayout_);
    }

    void ContactList::showNoSearchResults()
    {
        if (Logic::getSearchModelDLG()->count() == 0)
            showNoSearchResults(searchPage_, searchView_, searchLayout_);
    }

    void ContactList::showNoContactsYet()
    {
        showNoContactsYet(contactListPage_, contactListView_, contactListLayout_);
    }

    void ContactList::showNoRecentsYet()
    {
        if (Logic::getRecentsModel()->rowCount() != 0 || Logic::getUnknownsModel()->itemsCount() != 0)
            return;

        showNoRecentsYet(recentsPage_, recentsView_, recentsLayout_, [this]()
                         {
                             changeTab(CurrentTab::ALL);
                             Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_empty_write_msg);
                         });
    }

    void ContactList::typingStatus(Logic::TypingFires _typing, bool _isTyping)
    {
        if (recentsView_->model() == Logic::getRecentsModel() && Logic::getRecentsModel()->contactIndex(_typing.aimId_).isValid())
        {
            if (_isTyping)
                recentsDelegate_->addTyping(_typing);
            else
                recentsDelegate_->removeTyping(_typing);

            auto modeIndex = Logic::getRecentsModel()->contactIndex(_typing.aimId_);
            Logic::getRecentsModel()->dataChanged(modeIndex, modeIndex);
        }
    }

    void ContactList::messagesReceived(QString _aimId, QVector<QString> _chatters)
    {
        auto contactItem = Logic::getContactListModel()->getContactItem(_aimId);
        if (!contactItem)
            return;

        if (recentsView_->model() == Logic::getRecentsModel())
        {
            if (contactItem->is_chat())
                for (auto chatter: _chatters)
                    recentsDelegate_->removeTyping(Logic::TypingFires(_aimId, chatter, ""));
            else
                recentsDelegate_->removeTyping(Logic::TypingFires(_aimId, "", ""));

            auto modelIndex = Logic::getRecentsModel()->contactIndex(_aimId);
            Logic::getRecentsModel()->dataChanged(modelIndex, modelIndex);
        }
        else
        {
            auto modelIndex = Logic::getUnknownsModel()->contactIndex(_aimId);
            Logic::getUnknownsModel()->dataChanged(modelIndex, modelIndex);
        }
    }

    void ContactList::setEmptyIgnoreLabelVisible(bool _isVisible)
    {
        emptyIgnoreListLabel_->setVisible(_isVisible);
    }

    void ContactList::setClDelegate(Logic::ContactListItemDelegate* _delegate)
    {
        clDelegate_ = _delegate;
        contactListView_->setItemDelegate(_delegate);
        searchView_->setItemDelegate(_delegate);
    }

    void ContactList::setTransparent(bool _transparent)
    {
        if (_transparent)
            Utils::ApplyStyle(stackedWidget_, CL_TRANSPARENT);
    }

    void ContactList::clearSettingsSelection()
    {
        settingsTab_->cleanSelection();
    }

    void ContactList::selectSettingsVoipTab()
    {
        settingsTab_->settingsVoiceVideoClicked();
    }
    bool ContactList::getPictureOnlyView() const
    {
        return pictureOnlyView_;
    }

    void ContactList::setPictureOnlyView(bool _isPictureOnly)
    {
        if (pictureOnlyView_ == _isPictureOnly)
            return;

        pictureOnlyView_ = _isPictureOnly;
        recentsDelegate_->setPictOnlyView(pictureOnlyView_);
        unknownsDelegate_->setPictOnlyView(pictureOnlyView_);
        recentsView_->setFlow(QListView::TopToBottom);

        Logic::getRecentsModel()->setFavoritesHeadVisible(!pictureOnlyView_);
        Logic::getUnknownsModel()->setDeleteAllVisible(!pictureOnlyView_);

        if (pictureOnlyView_)
        {
            changeTab(RECENTS);
            setSearchMode(false);
            if (!Logic::getRecentsModel()->isFavoritesVisible())
                Logic::getRecentsModel()->toggleFavoritesVisible();
        }
        recentOrderChanged();
    }

    void ContactList::setButtonsVisibility(bool _isShow)
    {
         buttonsFrame_->setVisible(_isShow);
    }

    void ContactList::setItemWidth(int _newWidth)
    {
        fixedItemWidth_ = _newWidth;
        recentsDelegate_->setFixedWidth(_newWidth);
        searchItemDelegate_->setFixedWidth(_newWidth);
        unknownsDelegate_->setFixedWidth(_newWidth);
        clDelegate_->setFixedWidth(_newWidth);
    }

    QString ContactList::getSelectedAimid() const
    {
        QModelIndexList indexes;
        if (isSearchMode())
            indexes = searchView_->selectionModel()->selectedIndexes();
        else
            indexes = contactListView_->selectionModel()->selectedIndexes();

        QModelIndex index;
        for (const auto &index : indexes)
        {
            return getAimid(index);
        }

        return QString();
    }

    void ContactList::onDisableSearchInDialogButton()
    {
        emit Utils::InterConnector::instance().disableSearchInDialog();
        emit Utils::InterConnector::instance().repeatSearch();

        setSearchInDialog(false);
    }

    void ContactList::setSearchInDialog(bool _isSearchInDialog)
    {
        if (isSearchInDialog_ == _isSearchInDialog)
            return;

        isSearchInDialog_ = _isSearchInDialog;
        searchInAllButton_->setVisible(_isSearchInDialog);
        searchInChatLabel_->setVisible(_isSearchInDialog);
    }

    bool ContactList::getSearchInDialog() const
    {
        return isSearchInDialog_;
    }

    void ContactList::dialogClosed(QString _aimid)
    {
        auto searchModel =  qobject_cast<Logic::SearchModelDLG*>(Logic::getCurrentSearchModel(Logic::MembersWidgetRegim::CONTACT_LIST));
        auto chatAimid = searchModel->getDialogAimid();

        if (chatAimid == _aimid)
        {
            emit searchEnd();
        }
    }
}

namespace Logic
{
    bool is_delete_members_regim(int _regim)
    {
        return _regim == Logic::MembersWidgetRegim::DELETE_MEMBERS || _regim == Logic::MembersWidgetRegim::IGNORE_LIST;
    }

    bool is_admin_members_regim(int _regim)
    {
        return _regim == Logic::MembersWidgetRegim::ADMIN_MEMBERS;
    }
}
