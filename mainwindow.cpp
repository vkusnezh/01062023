#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "clickablelabel.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QImage>
#include <QVector>
#include <QQueue>
#include <QApplication>
#include <QLabel>
#include <QPainter>
#include <QPoint>
#include <QColor>
#include <QRgb>
#include <QMouseEvent>
#include <QDebug>
#include <QPaintEvent>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // Connect clicked signal of label_pic2 to slot mousePressedSlot
    connect(ui->label_pic2, &ClickableLabel::clicked, this, &MainWindow::mousePressedSlot);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// slot for handling the mouse pressed event on the label_pic2
void MainWindow::mousePressedSlot(QMouseEvent *event)
{
    // check if the click position is within the boundaries of the image
    QRect labelRect = ui->label_pic2->geometry();
    if (!labelRect.contains(event->pos())){
        return;
    }

    if (event->button() == Qt::LeftButton){
        // get the position of the click
        QPoint clickPosition = event->pos();

        // clear the previously selected segment
        selectedSegment.clear();

        // add the click position to the selected segment
        selectedSegment.append(clickPosition);

        // display the click position
        QString pointString = QString("%1, %2").arg(clickPosition.x()).arg(clickPosition.y());
        ui->pDark_2->setText(pointString);

        // calculate the perimeter coordinates and area of the segment
        calculateSegmentProperties();

        // update the QLabel to draw the selected segment
        ui->label_pic2->update();
    }
}

// calculate the properties of the selected segment (perimeter, area)
void MainWindow::calculateSegmentProperties()
{
    // expand the selected segment to neighboring pixels with the same color
    QColor clickColor = segmentedImage.pixel(selectedSegment.first());
    expandSegment(clickColor);

    // calculate the area of the segment (in % of the total image area)
    int imageArea = segmentedImage.width() * segmentedImage.height();
    float segmentArea = (static_cast<float>(selectedSegment.size()) / imageArea) * 100.0f;

    // calculate the coordinates of the segment edge
    QString segmentEdge;
    for (const QPoint &point : selectedSegment){
        segmentEdge.append(QString("X=%1, Y=%2\n").arg(point.x()).arg(point.y()));
    }

    // display the calculated area and segment edge
    ui->pDark_3->setText(QString::number(segmentArea));
    ui->pDark_4->setText(segmentEdge);
}

// expand the selected segment to neighboring pixels with the same color
void MainWindow::expandSegment(const QColor &color)
{
    QQueue<QPoint> queue;
    for (const QPoint &point : selectedSegment){
        queue.enqueue(point);
    }

    while (!queue.isEmpty()){
        QPoint currentPos = queue.dequeue();
        if (selectedSegment.contains(currentPos)){
            continue; // skip if already processed
        }

        // add the current position to the selected segment
        selectedSegment.append(currentPos);

        // get the color of the current position
        QColor currentColor = segmentedImage.pixel(currentPos);

        // check if the color matches the clicked color
        if (currentColor == color){
            // check neighboring pixels (up, down, left, right)
            QPoint neighbors[] = {
                QPoint(currentPos.x(), currentPos.y() - 1), // up
                QPoint(currentPos.x(), currentPos.y() + 1), // down
                QPoint(currentPos.x() - 1, currentPos.y()), // left
                QPoint(currentPos.x() + 1, currentPos.y()) // right
            };

            for (const QPoint &neighbor : neighbors){
                // check if the neighbor is within the image boundaries
                if (neighbor.x() >= 0 && neighbor.x() < segmentedImage.width() &&
                    neighbor.y() >= 0 && neighbor.y() < segmentedImage.height()){
                    // check if the neighbor is not already in the selected segment
                    if (!selectedSegment.contains(neighbor)){
                        // enqueue the neighbor for processing
                        queue.enqueue(neighbor);
                    }
                }
            }
        }
    }
}


// read and process the Image
void MainWindow::on_pushButton_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::homePath(), tr("Images (*.png *.xpm *.jpg)"));

    if (!file_name.isEmpty())
    {
        // open prompt and display image
        QMessageBox::information(this, "...", file_name);
        QImage img(file_name);
        QPixmap pix = QPixmap::fromImage(img);


        // get label dimensions
        int w = ui->label_pic->width();
        int h = ui->label_pic->height();
        // load image into the UI
        ui->label_pic->setPixmap(pix.scaled(w, h, Qt::KeepAspectRatio));

        // get image width and height, create empty binary matrix
        unsigned int cols = img.width();
        unsigned int rows = img.height();
        unsigned int numBlackPixels = 0;
        QVector<QVector<int>> imgArray(rows, QVector<int>(cols, 0));

        // get pixel data and update matrix
        for (unsigned int i = 0; i < rows; i++){
            for (unsigned int j = 0; j < cols; j++){
                // img.pixel(x,y) where x = col, y = row (coordinates)
                QColor clrCurrent(img.pixel(j, i));
                int r = clrCurrent.red();
                int g = clrCurrent.green();
                int b = clrCurrent.blue();
                int a = clrCurrent.alpha();
                // if black, assign 1
                // black r = 0, g = 0, b = 0, a = 255
                if (r + g + b < 20 && a > 240){
                    imgArray[i][j] = 1;
                    numBlackPixels += 1;
                }
            }
        }

        // update UI with information
        ui->dims->setText(QString("W: %1  H: %2").arg(cols).arg(rows));
        float pD = (static_cast<float>(numBlackPixels) / (cols * rows)) * 100.0f;
        ui->pDark->setText(QString::number(pD));

        // image segmentation
        QPixmap originalPixmap(file_name);
        QImage originalImage = originalPixmap.toImage(); // convert QPixmap to QImage for segmentation

        // create segmented image with the same dimensions as the original image
        segmentedImage = QImage(originalImage.size(), QImage::Format_ARGB32);

        // iterate over the pixels of the original image
        for (int y = 0; y < originalImage.height(); ++y){
            for (int x = 0; x < originalImage.width(); ++x){
                QRgb pixelColor = originalImage.pixel(x, y);

                // perform color-based segmentation
                // define the segmentation criteria based on the pixel color
                int red = qRed(pixelColor);
                int green = qGreen(pixelColor);
                int blue = qBlue(pixelColor);

                bool isSegment = false;
                // example segmentation criteria: segment if the sum of RGB components is below a threshold
                int sumThreshold = 550;
                int sum = red + green + blue;
                if (sum < sumThreshold){
                    isSegment = true;
                }

                if (isSegment){
                    // set the pixel in the segmented image to the original color
                    segmentedImage.setPixel(x, y, pixelColor);
                }
                else{
                    // set the pixel in the segmented image to a different color (e.g., black)
                    segmentedImage.setPixel(x, y, qRgb(0, 0, 0));
                }
            }
        }

        // create new QPixmap from the segmented image
        QPixmap segmentedPixmap = QPixmap::fromImage(segmentedImage);

        // display the segmented image
        ui->label_pic2->setPixmap(segmentedPixmap.scaled(w, h, Qt::KeepAspectRatio));
        ui->label_pic2->show();
    }
}

