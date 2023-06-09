#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QImage>
#include <QVector>
#include <QPoint>
#include <QColor>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();
    void mousePressEvent(QMouseEvent *event);
    void calculateSegmentProperties();
    void expandSegment(const QColor& color);
    //void on_pushButton_2_clicked();

private:
    Ui::MainWindow *ui;
    QImage originalImage;  // Original image
    QImage segmentedImage; // Segmented image
    QPixmap segmentedPixmap;
    QVector<QPoint> selectedSegment; // Selected segment
};

#endif // MAINWINDOW_H
