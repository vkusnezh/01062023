#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "clickablelabel.h"
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

public slots:


private slots:
    void on_pushButton_clicked();
 //   void mousePressedSlot(QMouseEvent *event);
    void mousePressedSlot(const QPoint& pos);
//    void calculateSegmentProperties();
//    void expandSegment(const QColor& color);
    QVector<QPoint> expandSegment(const QPoint& labelPos);
    void calculateSegmentProperties(const QVector<QPoint>& perimeterCoordinates);
    bool eventFilter(QObject* object, QEvent* event);

private:
    Ui::MainWindow *ui;
    ClickableLabel *label_pic2; // Declare ClickableLabel pointer member variable
    QLabel label_pic_2;
    QImage originalImage;  // Original image
    QImage segmentedImage; // Segmented image
    QPixmap segmentedPixmap;
//    QVector<QPoint> selectedSegment; // Selected segment
    QVector<QPoint> perimeterCoordinates;
    QPoint labelPos;
};



#endif // MAINWINDOW_H
