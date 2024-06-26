#ifndef SEARCHINFILESDIALOG_H
#define SEARCHINFILESDIALOG_H

#include <QDialog>
#include <QTreeWidgetItem>
#include <QAtomicInteger>
#include <QMutex>

#include "dltmessagefinder.h"

namespace Ui {
class SearchInFilesDialog;
}

class SearchInFilesDialog : public QDialog
{
    Q_OBJECT

    Ui::SearchInFilesDialog *ui;
    DltMessageFinder    *multiFileSearcher;
    QString              m_currentPath;
    QTreeWidgetItem      tree_results;
    QStringList          m_files;
    QAtomicInteger<bool> m_indexing;
    QMutex               m_indexingMutex;
    long                 totalMatches = 0;

    QStringList getQueryPatterns();

    void initConnections();

    void on_pushButtonLoadConfig_clicked(bool replace = true);

public:
    explicit SearchInFilesDialog(QWidget *parent = nullptr);
    ~SearchInFilesDialog();
    SearchInFilesDialog* setFolder(const QString &path);

signals:
    void openDltFiles(const QStringList& files);

public slots:
    void dltFilesOpened(const QStringList& files);

private slots:
    void on_buttonSearch_clicked();

    void on_buttonCancel_clicked();

    void on_buttonAddPattern_clicked();

    void on_buttonRemovePattern_clicked();

    void on_directoryTextBox_textChanged(const QString &arg1);

    void on_directoryTextBox_returnPressed();

    void on_treeWidgetResults_itemClicked(QTreeWidgetItem *item, int column);

    void on_treeWidgetResults_customContextMenuRequested(const QPoint &pos);

    void on_pushButtonSelectAll_clicked();

    void on_pushButtonDeselectAll_clicked();

    void on_comboBoxSelectedResults_activated(int index);

    void on_pushButtonSaveConfig_clicked();

protected:
    void closeEvent(QCloseEvent *event) override
    {
        (void)event;
//        multiFileSearcher->cancelSearch(true); /* Clear cache memeory, also */
        close();
    }
    void showEvent(QShowEvent *event) override;
};

#endif // SEARCHINFILESDIALOG_H
