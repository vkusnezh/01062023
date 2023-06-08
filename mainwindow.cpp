#include "mainwindow.h"
#include "ui_mainwindow.h"
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
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
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
                    numBlackPixels+=1;
                }
            }
        }


        QString filename = "C:/Users/38063/Desktop/Infineon_/temp.txt";
        QFile fileout(filename);
        if (fileout.open(QFile::ReadWrite | QFile::Text))
        {
            QTextStream out(&fileout);
            // loop through 2d array and add
            for (unsigned int i = 0; i < rows; i++)
            {
                for (unsigned int j = 0; j < cols; j++)
                {
                    out << imgArray[i][j];
                }
                // end line after each column, go to next row
                out << " " << Qt::endl;
            }
        }

        // update UI with information, label text must be a Qt string
        ui->dims->setText(QString::fromStdString("W: " + std::to_string(cols) + "  H: " + std::to_string(rows)));
        float pD = ((float)numBlackPixels/(float)(cols*rows))*100;
        ui->pDark->setText(QString::fromStdString(std::to_string(pD)));



        // Image segmentation
        QPixmap originalPixmap(file_name);
        QImage originalImage = originalPixmap.toImage(); // Convert QPixmap to QImage for segmentation

        // Create a segmented image with the same dimensions as the original image
        QImage segmentedImage = QImage(originalImage.size(), QImage::Format_ARGB32);

        // Iterate over the pixels of the original image
        for (int y = 0; y < originalImage.height(); ++y) {
            for (int x = 0; x < originalImage.width(); ++x) {
                QRgb pixelColor = originalImage.pixel(x, y);

                // Perform color-based segmentation
                // Define the segmentation criteria based on the pixel color
                int red = qRed(pixelColor);
                int green = qGreen(pixelColor);
                int blue = qBlue(pixelColor);

                bool isSegment = false;
                // Example segmentation criteria: segment if red component is greater than green and blue components
                if (red > green && red > blue) {
                    isSegment = true;
                }

                if (isSegment) {
                    // Set the pixel in the segmented image to the original color
                    segmentedImage.setPixel(x, y, pixelColor);
                } else {
                    // Set the pixel in the segmented image to a different color (e.g., black)
                    segmentedImage.setPixel(x, y, qRgb(0, 0, 0));
                }
            }
        }

        // Create a new QPixmap from the segmented image
        QPixmap segmentedPixmap = QPixmap::fromImage(segmentedImage);

        // Display the segmented image
        ui->label_pic2->setPixmap(segmentedPixmap.scaled(w, h, Qt::KeepAspectRatio));
        ui->label_pic2->show();
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    // Define variables to store the selected segment
    QVector<QPoint> selectedSegment;
    double segmentArea = 0.0;

    if (event->button() == Qt::LeftButton) {
        // Get the position of the click
        QPoint clickPosition = event->pos();

        // Check if the click position is within the boundaries of the image
        QRect labelRect = ui->label_pic2->geometry();
        if (labelRect.contains(clickPosition)) {
            // Clear the previously selected segment
            selectedSegment.clear();

            QString pointString = QString::number(clickPosition.x()) + ", " + QString::number(clickPosition.y());
            ui->pDark_2->setText(pointString);

            // Add the click position to the selected segment
            selectedSegment.append(clickPosition);

            // Calculate the perimeter coordinates
            QString varText;
            for (const QPoint& point : selectedSegment) {
//                stream() << "X:" << point.x() << ", Y:" << point.y();
                varText.append(QString("X=%1, Y=%2").arg(point.x()).arg(point.y()));
            }

            // Calculate the area of the segment (in % of the total image area)
            int imageArea = segmentedImage.width() * segmentedImage.height();
            segmentArea = (static_cast<double>(selectedSegment.size()) / imageArea) * 100.0;

            // Display the calculated area
            ui->pDark_3->setText(QString::fromStdString(std::to_string(segmentArea)));

            ui->pDark_4->setText(varText);

            // Update the QLabel to draw the selected segment
            ui->label_pic2->update();
        }
    }
}

void MainWindow::paintEvent(QPaintEvent *event) {
    QMainWindow::paintEvent(event);

    // Get the position of the QLabel relative to its parent widget
    QPoint labelPosition = ui->label_pic2->mapToParent(QPoint(0, 0));

    QPainter painter(this);
    painter.setPen(Qt::red);

    for (int i = 1; i < selectedSegment.size(); ++i) {
        // Adjust the coordinates based on the position of the QLabel
        QPoint startPoint = selectedSegment[i - 1] + labelPosition;
        QPoint endPoint = selectedSegment[i] + labelPosition;
        painter.drawLine(startPoint, endPoint);
    }
}
