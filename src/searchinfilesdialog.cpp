#include <QDebug>
#include <QDirIterator>
#include <QCheckBox>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QDateTime>
#include <QStandardItemModel>
#include <QFileDialog>
#include <QClipboard>
#include <QSettings>

#include "ui_searchinfilesdialog.h"
#include "searchinfilesdialog.h"

#include "dlt_common.h"

typedef enum
{
    RESULT_INDEX_COL = 0,
    RESULT_TIME_COL,
    RESULT_TIMEST_COL,
    RESULT_ECUID_COL,
    RESULT_APPID_COL,
    RESULT_CTXID_COL,
    RESULT_PAYLD_COL 
}e_ResultsTableColumn;

SearchInFilesDialog::SearchInFilesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SearchInFilesDialog),
    multiFileSearcher(DltMessageFinder::getInstance(this))
{
    ui->setupUi(this);

    ui->treeWidgetResults->setColumnWidth(0, ui->treeWidgetResults->width() - 65);
    ui->treeWidgetResults->setColumnWidth(1, 60);

    ui->tableWidgetResults->setColumnWidth(RESULT_TIME_COL, 130);
    ui->tableWidgetResults->setColumnWidth(RESULT_TIMEST_COL, 90);
    ui->tableWidgetResults->setColumnWidth(RESULT_PAYLD_COL, 1600);
    ui->tableWidgetResults->horizontalHeaderItem(RESULT_PAYLD_COL)->setTextAlignment(Qt::AlignLeft);

    /* enable custom context menu */
    ui->treeWidgetResults->setContextMenuPolicy(Qt::CustomContextMenu);

    /* Dont show progress bar at start */
    ui->progressBarSearch->hide();

    QMenu* menu = new QMenu(this);
    menu->addAction(tr("Replace"), [this](){
        on_pushButtonLoadConfig_clicked(true);
    });
    menu->addAction(tr("Append"), [this](){
        on_pushButtonLoadConfig_clicked(false);
    });

    ui->pushButtonLoadConfig->setMenu(menu);

    /* Make connections */
    initConnections();

    /* Disable selection for first item in dropdown menu */
    qobject_cast<QStandardItemModel*>(ui->comboBoxSelectedResults->model())->item(0)->setEnabled(false);
}

void SearchInFilesDialog::initConnections()
{
    connect(multiFileSearcher, &DltMessageFinder::startedSearch, this, [this](){
        // set search button disabled
        ui->buttonSearch->setEnabled(false);
        ui->buttonCancel->setText("Stop");
        /* disable textbox */
        ui->directoryTextBox->setEnabled(false);
        ui->tabWidget->setCurrentIndex(1);
    });

    connect(multiFileSearcher, &DltMessageFinder::stoppedSearch, this, [this](bool full_stop){
        // set search button enabled
        ui->buttonCancel->setText("Close");
        ui->buttonSearch->setEnabled(true);
        /* enable textbox */
        ui->directoryTextBox->setEnabled(true);

        if (full_stop)
        {
            ui->groupBoxSearchResult->setTitle("Search results:");

            ui->pushButtonDeselectAll->setEnabled(false);
            ui->pushButtonSelectAll->setEnabled(false);

            ui->treeWidgetResults->clear();
            ui->tableWidgetResults->setRowCount(0);

            totalMatches = 0;
        }
    });

    connect(multiFileSearcher, &DltMessageFinder::searchFinished, this, [this](){
        // set search button enabled
        ui->buttonSearch->setEnabled(true);
        ui->buttonCancel->setText("Close");
        /* enable textbox */
        ui->directoryTextBox->setEnabled(true);

        /* hide progressbar when done */
        ui->progressBarSearch->hide();
    });

    connect(multiFileSearcher, &DltMessageFinder::foundFile, this, [this](int index){
        /* get path from results */
        /*
        qDebug() << QString("[%1] + parent: %2")
                        .arg(QDateTime::currentMSecsSinceEpoch())
                        .arg(QString::number(index));
        */
        QTreeWidgetItem* item     = new QTreeWidgetItem;
        QTreeWidgetItem* sub_item = new QTreeWidgetItem;

        auto result = multiFileSearcher->getResults().at(index);
        QString path = result->first->getFileName();

        item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
        item->setCheckState(0, Qt::Unchecked);

        item->setText(0, path);
        item->setToolTip(0, path);
        item->setText(1, "1");

        auto message = QString("index: " ) + QString::number(result->second.at(0));
        sub_item->setText(0, "        "+message);
        sub_item->setToolTip(0, "Click to display results at " + message);
        item->addChild(sub_item);

        if (0u == ui->treeWidgetResults->topLevelItemCount())
        {
            ui->pushButtonDeselectAll->setEnabled(true);
            ui->pushButtonSelectAll->setEnabled(true);
        }

        ui->treeWidgetResults->insertTopLevelItem(
                    ui->treeWidgetResults->topLevelItemCount(), item);

        auto newTitle = QString("Search results: %1 matches in %2 files").arg(totalMatches++)
                .arg(ui->treeWidgetResults->topLevelItemCount());
        ui->groupBoxSearchResult->setTitle(newTitle);
    });

    connect(multiFileSearcher, &DltMessageFinder::resultPartial, this, [this](int f_index, int index){
#if 0
        qDebug() << QString("[%1] + parent: %2 child: %3")
                        .arg(QDateTime::currentMSecsSinceEpoch())
                        .arg(QString::number(f_index),
                             QString::number(index));
#endif
        auto items = ui->treeWidgetResults->topLevelItemCount();

        if (items > 0)
        {
            auto result     = multiFileSearcher->getResults().at(f_index);
            auto tree_item  = ui->treeWidgetResults->topLevelItem(f_index);
            auto sub_item   = new QTreeWidgetItem;

            tree_item->addChild(sub_item);
            tree_item->setText(1, QString::number(result->second.size()));
            auto message = "index: " + QString::number(result->second.at(index));
            sub_item->setText(0, "        "+message);
            sub_item->setToolTip(0, "Click to display results at " + message);
        }
        auto newTitle = QString("Search results: %1 matches in %2 files").arg(totalMatches++)
                .arg(ui->treeWidgetResults->topLevelItemCount());
        ui->groupBoxSearchResult->setTitle(newTitle);
    });

    connect(multiFileSearcher, &DltMessageFinder::processedFile, this, [this](QString){
        ui->progressBarSearch->setValue(ui->progressBarSearch->value()+1);
    });
}

SearchInFilesDialog::~SearchInFilesDialog()
{
    m_indexing = false;
    m_indexingMutex.lock(); /* In order to avoid destroing of locked mutex */
    m_indexingMutex.unlock();

    delete ui;
}

SearchInFilesDialog* SearchInFilesDialog::setFolder(const QString &path)
{
    m_currentPath = path;
    ui->directoryTextBox->setText(m_currentPath);

    return this;
}

void SearchInFilesDialog::dltFilesOpened(const QStringList &files)
{
(void)files;
}

QStringList SearchInFilesDialog::getQueryPatterns()
{
    QStringList expressions;
    QTableWidget  *table = ui->tableWidgetPatterns;

    /* Compose search query based on Patterns Table */
    for (int i = 0; i < table->rowCount(); i++)
    {
        /* Check if query is enabled */
        bool is_enabled = dynamic_cast<QCheckBox*>(table->cellWidget(i, 0))->isChecked();

        if (is_enabled)
        {
            ;
            auto text = table->item(i, 1)->text();
            if (!text.isEmpty())
            {
                /* test if regex expression is valid */
                expressions << ("("+text+")");
            }
            else
                qDebug() << "empty";
        }

    }

    return expressions;
}

void SearchInFilesDialog::on_buttonSearch_clicked()
{
    if (!QDir(m_currentPath).exists())
    {
        /* Dialog, folder not exists */
        return ;
    }

    /* if there are some, clear previous results */
    multiFileSearcher->cancelSearch(true);
    ui->treeWidgetResults->clear();
    ui->labelResultFileName->clear();

    if (!multiFileSearcher->isRunning())
    {
        QDltFilterList filters;
        QStringList    expressions = getQueryPatterns();

        if (expressions.empty())
        {
            /* Dialog that query is empty */
            return;
        }

        if (m_files.size() > 0)
        {
            ui->progressBarSearch->reset();
            ui->progressBarSearch->setMaximum(m_files.size());
            ui->progressBarSearch->setValue(0);
            ui->progressBarSearch->show();

            multiFileSearcher->search(m_files, std::move(expressions.join("|")));
        }
        else
        {
            QMessageBox warning(QMessageBox::Warning,
                                "Warning", "No dlt files in selected directory",
                                QMessageBox::Ok);
            warning.exec();
        }
    }
}

void SearchInFilesDialog::on_buttonCancel_clicked()
{
    ui->progressBarSearch->hide();

    if (multiFileSearcher->isRunning())
    {/* This will also re-enable the search button */
        multiFileSearcher->cancelSearch(false);/* Stop the search, but keep results */
    }
    else
    {
        multiFileSearcher->cancelSearch(true); /* Clear cache memeory, also */
        ui->labelResultFileName->clear();
        close();
    }
}

void SearchInFilesDialog::on_buttonAddPattern_clicked()
{
    auto rowIdx = ui->tableWidgetPatterns->rowCount();
    auto checkBox = new QCheckBox(ui->tableWidgetPatterns);
    auto table = ui->tableWidgetPatterns;

    checkBox->setCheckState(Qt::Checked);
    checkBox->setStyleSheet("margin-left:25%;");
    table->insertRow(rowIdx);
    table->setCellWidget(rowIdx, 0, checkBox);

    auto item = new QTableWidgetItem("");
    item->setText("");
    table->setItem(rowIdx, 1, item);
    table->setFocus();
}

void SearchInFilesDialog::on_buttonRemovePattern_clicked()
{
    auto table = ui->tableWidgetPatterns;
    auto sel_m = table->selectionModel();
    QVector<int> indexes;

    foreach(auto index, sel_m->selectedRows())
    {
        indexes.append(index.row());
    }
    int len = indexes.size();
    while(len-- > 0)
    {
        table->removeRow(indexes.at(len));
    }
}

void SearchInFilesDialog::on_directoryTextBox_textChanged(const QString &arg1)
{
    m_currentPath = arg1;

    if (QDir(m_currentPath).exists())
    {
        m_indexing = false;

        QThreadPool::globalInstance()->start([this](){
            m_indexingMutex.lock();

            QDirIterator dir_it(m_currentPath, QStringList() << "*.dlt", QDir::Files, QDirIterator::Subdirectories);
            int total_files = 0;

            m_files.clear();
            ui->buttonSearch->setEnabled(false);
            ui->buttonSearch->setText("Indexing files");

            m_indexing = true;

            while (dir_it.hasNext() && m_indexing)
            {
                m_files.append(dir_it.next());
                total_files++;
            }

            if (!m_indexing)
            {
                qDebug() << "Indexing interrupted";
            }

            ui->buttonSearch->setEnabled(true);
            ui->buttonSearch->setText("Search");

            m_indexingMutex.unlock();
        });
    }
}


void SearchInFilesDialog::on_directoryTextBox_returnPressed()
{
}

void SearchInFilesDialog::showEvent(QShowEvent *event)
{
    (void)event;
    ui->tabWidget->setCurrentIndex(0);
}

QTableWidgetItem* insertResultRow(QTableWidget *resultsTable, QDltMsg &message, int idx, int msg_id = 0)
{
    auto item    = new QTableWidgetItem;

    resultsTable->insertRow(idx);

    /* payload */
    item->setText(message.toStringPayload());
    resultsTable->setItem(idx, RESULT_PAYLD_COL, item);

    /* time */
    item = new QTableWidgetItem;
    QDateTime dt;
    dt.setTime_t(message.getTime());
    item->setText(dt.toString("yyyy-MM-dd hh:mm:ss"));
    resultsTable->setItem(idx, RESULT_TIME_COL, item);

    /* timestamp */
    item = new QTableWidgetItem;
    item->setText(QString::number(message.getTimestamp()/10000.));
    resultsTable->setItem(idx, RESULT_TIMEST_COL, item);

    /* ecuid */
    item = new QTableWidgetItem;
    item->setText(message.getEcuid());
    resultsTable->setItem(idx, RESULT_ECUID_COL, item);

    /* apid */
    item = new QTableWidgetItem;
    item->setText(message.getApid());
    resultsTable->setItem(idx, RESULT_APPID_COL, item);

    /* ctid */
    item = new QTableWidgetItem;
    item->setText(message.getCtid());
    resultsTable->setItem(idx, RESULT_CTXID_COL, item);

    /* index */
    item = new QTableWidgetItem;
    item->setText(QString::number( msg_id ));
    resultsTable->setItem(idx, RESULT_INDEX_COL, item);

    return item;
}

void SearchInFilesDialog::on_treeWidgetResults_itemClicked(QTreeWidgetItem *item, int column)
{
    (void)column;
    auto resultsTable = ui->tableWidgetResults;
    int topLvlIndex = ui->treeWidgetResults->indexOfTopLevelItem(item);
    std::shared_ptr<QDltFile>dltFile;

    /* Delete results table items */
    if (resultsTable->rowCount() > 0)
    {
        resultsTable->setRowCount(0);
        resultsTable->scrollToTop();
    }

    if (-1 != topLvlIndex)
    {
        /* ToDo: Create a table with all results from the file and display it as usual */
        auto results      = multiFileSearcher->getResults().at(topLvlIndex);
        auto numResults   = results->second.size();
        dltFile           = results->first;

        for (int idx = 0; idx < numResults; idx++)
        {
            auto msg_idx = results->second.at(idx);
            QDltMsg message;

            /* get message from file and insert it in the table */            
            dltFile->getMsg(msg_idx, message);
            insertResultRow(resultsTable, message, idx, msg_idx);
        }

        resultsTable->setFocus();
    }
    else
    {/* it means, a child was clicked */
        auto parent = item->parent();
        auto childIndex = 0;

        topLvlIndex = ui->treeWidgetResults->indexOfTopLevelItem(parent);
        childIndex  = parent->indexOfChild(item);

        auto results      = multiFileSearcher->getResults().at(topLvlIndex);
        /* Get .dlt file content for current index */
        dltFile      = results->first;

        QTableWidgetItem* ourItem = nullptr;
        long ourItemIndex = 0;
        long result_idx   = results->second.at(childIndex);
        long msgNumber    = dltFile->getFileMsgNumber();
        long start        = result_idx - 100;
        long end          = result_idx + 100;

        start = (start >= 0)?start:0;
        end   = (end <= msgNumber)?end:msgNumber;

        for (long idx = 0, msg_idx = start; msg_idx < end; idx++, msg_idx++)
        {
            QTableWidgetItem* item;
            QDltMsg message;

            dltFile->getMsg(msg_idx, message);
            item = insertResultRow(resultsTable, message, idx, msg_idx);

            if (result_idx == msg_idx)
            {
                ourItem      = item;
                ourItemIndex = idx;
            }
        }

        /* Scroll to message idx: at(childIndex) */
        resultsTable->setFocus();
        resultsTable->scrollToItem(ourItem, QAbstractItemView::PositionAtCenter);
        resultsTable->setRangeSelected({ourItemIndex, 0, ourItemIndex, 4}, true);
    }

    ui->labelResultFileName->setText(dltFile->getFileName());
}

void SearchInFilesDialog::on_treeWidgetResults_customContextMenuRequested(const QPoint &pos)
{
    if (multiFileSearcher->getResults().size() <= 0)
        return;

    auto resultsTree  = ui->treeWidgetResults;
    auto item         = resultsTree->itemAt(pos);
    int  topLvlIndex  = resultsTree->indexOfTopLevelItem(item);

    if (-1 == topLvlIndex)
    {/* item is a treeChild */
        auto parent = item->parent();

        topLvlIndex = resultsTree->indexOfTopLevelItem(parent);
    }

    auto results = multiFileSearcher->getResults().at(topLvlIndex);
    auto dltFile = results->first->getFileName();

    QMenu menu(ui->treeWidgetResults);
    auto action = new QAction("&Load", this);
    connect(action, &QAction::triggered, this, [this, dltFile](){
        emit openDltFiles(QStringList() << dltFile);
    });
    menu.addAction(action);

    menu.exec(resultsTree->mapToGlobal(pos));
}

void SearchInFilesDialog::on_pushButtonSelectAll_clicked()
{
    auto resultsTree = ui->treeWidgetResults;
    int  items_cnt = resultsTree->topLevelItemCount();

    for (int i = 0; i < items_cnt; i++)
    {
        resultsTree->topLevelItem(i)->setCheckState(0, Qt::Checked);
    }
}

void SearchInFilesDialog::on_pushButtonDeselectAll_clicked()
{
    auto resultsTree = ui->treeWidgetResults;
    int  items_cnt = resultsTree->topLevelItemCount();

    for (int i = 0; i < items_cnt; i++)
    {
        resultsTree->topLevelItem(i)->setCheckState(0, Qt::Unchecked);
    }
}

void SearchInFilesDialog::on_comboBoxSelectedResults_activated(int index)
{
    auto resultsTree = ui->treeWidgetResults;
    int  items_cnt = resultsTree->topLevelItemCount();

    ui->comboBoxSelectedResults->setCurrentIndex(0);

    switch (index)
    {
    case 1: /* Load files */
    {
        QStringList files;

        for (int i = 0; i < items_cnt; i++)
        {
            auto results = multiFileSearcher->getResults().at(i);
            auto dltFile = results->first->getFileName();

            if (Qt::Checked == resultsTree->topLevelItem(i)->checkState(0))
            {
                files << dltFile;
            }
        }

        if (files.size() > 0)
        {
            emit openDltFiles(files);
        }

        break;
    }
    case 2: /* Save results */
    {
        QString fileName = QFileDialog::getSaveFileName(this, "Save results to file", m_currentPath, "*.txt");

        if (fileName != "")
        {
            QFile file(QFileInfo(fileName).absoluteFilePath());

            if (!file.open(QIODevice::WriteOnly))
            {
                QMessageBox::critical(this, tr("Error"), tr("Failed to save file"));
            }
            else
            {
                QTextStream outputText(&file);

                QString blockSeparator = QString("\n%1\n").arg(QString("-").repeated(100));

                outputText << QString("Search results\n\nBase folder: %1\nPatterns:\n%2\n\n")
                                .arg(QDir::toNativeSeparators(m_currentPath), getQueryPatterns().join("\n"));

                for (int i = 0; i < items_cnt; i++)
                {
                    auto results = multiFileSearcher->getResults().at(i);
                    auto dltFile = results->first;

                    if (Qt::Checked == resultsTree->topLevelItem(i)->checkState(0))
                    {
                        outputText << QString("File - %1:%2")
                                .arg(QDir::toNativeSeparators(dltFile->getFileName()),
                                     blockSeparator);

                        for (int i = 0; i < results->second.size(); i++)
                        {
                            int idx = results->second.at(i);
                            QDltMsg message;

                            dltFile->getMsg(idx, message);
                            outputText << QString("%1 %2 - %3\n").arg(idx)
                                          .arg(message.toStringHeader(),
                                               message.toStringPayload().replace("\n" ,"  "));
                        }

                        outputText << QString("%1\n").arg(blockSeparator);
                    }
                }

                file.close();
                QMessageBox::information(this, "Saving done!", "The results were successfully saved");
            }
        }

        break;
    }
    case 3: /* Search again */
    {
        QStringList files;

        for (int i = 0; i < items_cnt; i++)
        {
            auto results = multiFileSearcher->getResults().at(i);
            auto dltFile = results->first->getFileName();

            if (Qt::Checked == resultsTree->topLevelItem(i)->checkState(0))
            {
                files << dltFile;
            }
        }

        if (files.size() > 0)
        {
            m_files = files;
            on_buttonSearch_clicked();
        }

        break;
    }
    case 4: /* Copy paths */
    {
        QClipboard *clipboard = QGuiApplication::clipboard();
        QStringList files;

        for (int i = 0; i < items_cnt; i++)
        {
            auto results = multiFileSearcher->getResults().at(i);
            auto dltFile = results->first->getFileName();

            if (Qt::Checked == resultsTree->topLevelItem(i)->checkState(0))
            {
                files << dltFile;
            }
        }

        clipboard->setText(files.join("\n"));

        break;
    }
    default:
        break;
    }
}

void SearchInFilesDialog::on_pushButtonLoadConfig_clicked(bool replace)
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open patterns", m_currentPath, "Patterns (*.ini)");

    if (fileName != "")
    {
        QSettings settings(fileName, QSettings::IniFormat);
        QStringList patterns;

        int size = settings.beginReadArray("patterns");
        for (int i = 0; i < size; ++i)
        {
            settings.setArrayIndex(i);
            patterns.append(settings.value("patterns").toString());
        }
        settings.endArray();

        if (patterns.size() <= 0)
        {
            qDebug() << "Invalid patterns config file";
        }
        else
        {
            if (replace)
            {
                /* clear patterns from table */
                ui->tableWidgetPatterns->clearContents();
                ui->tableWidgetPatterns->setRowCount(0);
            }

            /* Write patterns to table */
            for (auto &pattern : patterns)
            {
                QTableWidgetItem *item = new QTableWidgetItem(pattern);

                /* insert new row */
                on_buttonAddPattern_clicked();
                /*write the pattern value*/
                ui->tableWidgetPatterns->setItem(ui->tableWidgetPatterns->rowCount()-1, 1, item);
            }
        }
    }
}


void SearchInFilesDialog::on_pushButtonSaveConfig_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save patterns", m_currentPath, "Patterns (*.ini)");

    if (fileName != "")
    {
        QSettings settings(fileName, QSettings::IniFormat);

        auto patterns = getQueryPatterns();
        settings.beginWriteArray("patterns");
        for (int i = 0; i < patterns.size(); ++i) {
            QString pattern = patterns.at(i);
            pattern = pattern.mid(1, pattern.size() - 2);

            settings.setArrayIndex(i);
            settings.setValue("patterns", pattern);
        }
        settings.endArray();
    }
}

