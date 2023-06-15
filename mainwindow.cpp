#include "mainwindow.h"
#include "ui_mainwindow.h"
//#include "clickablelabel.h"
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
#include <QGraphicsPixmapItem>
#include <QTime>
#include <QDateTime>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    label_pic2(nullptr) // Initialize the label_pic2 pointer to nullptr
{
    ui->setupUi(this);

    // In the constructor or initialization function
    ui->label_pic_2->installEventFilter(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::homePath(), tr("Images (*.png *.xpm *.jpg)"));

    if (!file_name.isEmpty())
    {
        // Open prompt and display the image
        QMessageBox::information(this, "...", file_name);
        QImage img(file_name);
        QPixmap pix = QPixmap::fromImage(img);

        // Get label dimensions
        int w = ui->label_pic->width();
        int h = ui->label_pic->height();

        // Load the image into the UI
        ui->label_pic->setPixmap(pix.scaled(w, h, Qt::KeepAspectRatio));

        // Get image width and height, create an empty binary matrix
        unsigned int cols = img.width();
        unsigned int rows = img.height();
        unsigned int numBlackPixels = 0;
        QVector<QVector<int>> imgArray(rows, QVector<int>(cols, 0));

        // Get pixel data and update the matrix
        for (unsigned int i = 0; i < rows; i++)
        {
            for (unsigned int j = 0; j < cols; j++)
            {
                // img.pixel(x,y) where x = col, y = row (coordinates)
                QColor clrCurrent(img.pixel(j, i));
                int r = clrCurrent.red();
                int g = clrCurrent.green();
                int b = clrCurrent.blue();
                int a = clrCurrent.alpha();

                // If black, assign 1
                // Black r = 0, g = 0, b = 0, a = 255
                if (r + g + b < 20 && a > 240)
                {
                    imgArray[i][j] = 1;
                    numBlackPixels += 1;
                }
            }
        }

        // Update UI with information
        ui->dims->setText(QString("W: %1  H: %2").arg(cols).arg(rows));
        float pD = (static_cast<float>(numBlackPixels) / (cols * rows)) * 100.0f;
        ui->pDark->setText(QString::number(pD));

        // Image segmentation
        QImage originalImage = img.copy();

        // Create a segmented image with the same dimensions as the original image
        segmentedImage = QImage(originalImage.size(), QImage::Format_ARGB32);

        // Iterate over the pixels of the original image
        for (int y = 0; y < originalImage.height(); ++y)
        {
            for (int x = 0; x < originalImage.width(); ++x)
            {
                QRgb pixelColor = originalImage.pixel(x, y);

                // Perform color-based segmentation
                // Define the segmentation criteria based on the pixel color
                int red = qRed(pixelColor);
                int green = qGreen(pixelColor);
                int blue = qBlue(pixelColor);

                bool isSegment = false;
                // Example segmentation criteria: segment if the sum of RGB components is below a threshold
                int sumThreshold = 550;
                int sum = red + green + blue;
                if (sum < sumThreshold)
                {
                    isSegment = true;
                }

                if (isSegment)
                {
                    // Set the pixel in the segmented image to the original color
                    //segmentedImage.setPixel(x, y, pixelColor);
                    segmentedImage.setPixel(x, y, qRgb(255, 255, 255));
                }
                else
                {
                    // Set the pixel in the segmented image to a different color (e.g., black)
                    segmentedImage.setPixel(x, y, qRgb(0, 0, 0));
                }
            }
        }

        // Load the image into the UI
        QPixmap segmentedPixmap = QPixmap::fromImage(segmentedImage);
        ui->label_pic_2->setPixmap(segmentedPixmap.scaled(w, h, Qt::KeepAspectRatio));

        // Set the alignment of label_pic_2 to top-left
        ui->label_pic_2->setAlignment(Qt::AlignTop | Qt::AlignLeft);
/*
        // Create an instance of ClickableLabel and assign it to label_pic2
        label_pic2 = new ClickableLabel(this);

        // Create a layout and set it as the central widget layout
        QHBoxLayout* layout = new QHBoxLayout(ui->centralwidget);
        layout->addWidget(label_pic2);
        //layout->setContentsMargins(70, 250, 300, 20);
        layout->setContentsMargins(0, 20, 0, 0);

        // Connect clicked signal of label_pic2 to slot mousePressedSlot
        connect(label_pic2, &ClickableLabel::mousePressed, this, &MainWindow::mousePressedSlot);
*/
    }
}


// Override the eventFilter function in MainWindow class
bool MainWindow::eventFilter(QObject* object, QEvent* event)
{
    if (object == ui->label_pic_2 && event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            QPoint pos = mouseEvent->pos();
            mousePressedSlot(pos); // Call mousePressedSlot directly with the position
        }
    }
    return QMainWindow::eventFilter(object, event);
}



void MainWindow::mousePressedSlot(const QPoint& pos)
{
    // Convert the click position to the coordinates of label_pic_2
    QPoint cursorPos = ui->label_pic_2->mapFromGlobal(ui->label_pic_2->mapToGlobal(pos));

    // Clear the previously selected segment
    //selectedSegment.clear();

    // Add the click position to the selected segment
    //selectedSegment.append(cursorPos);

    // Display the click position
    QString pointString = QString("%1, %2").arg(cursorPos.x()).arg(cursorPos.y());
    ui->pDark_2->setText(pointString);

    qDebug() << "Segmented Image Size:" << segmentedImage.size();

    // Calculate the perimeter coordinates and area of the segment
    QVector<QPoint> perimeterCoordinates = expandSegment(cursorPos);

    calculateSegmentProperties(perimeterCoordinates);

    // Update label_pic2 to draw the selected segment
    ui->label_pic_2->update();
}

QVector<QPoint> MainWindow::expandSegment(const QPoint& currentPos)
{   //Function uses a breadth-first search (BFS) algorithm to explore the neighboring pixels of the currentPos
    //and collects all the pixels with the same color as currentColor
    QVector<QPoint> perimeterCoordinates;
    QVector<QPoint> segmentCoordinates;
    QQueue<QPoint> queue;
    queue.enqueue(currentPos);

    // Get the color of the current position
    QColor currentColor = segmentedImage.pixelColor(currentPos);
    qDebug() << "MainWindow::expandSegment, Current Color:" << currentColor;

    if (currentColor == qRgb(0, 0, 0))
    {
        perimeterCoordinates.append(currentPos);
        return perimeterCoordinates;
    }

    while (!queue.isEmpty())
    {
        QPoint pos = queue.dequeue();
        if (segmentCoordinates.contains(pos))
        {
            continue; // Skip if already processed
        }

        // Add the current position to the segment coordinates
        segmentCoordinates.append(pos);

        // Check if the color matches the current color
        if (segmentedImage.pixelColor(pos) == currentColor)
        {
            // Check neighboring pixels (up, down, left, right)
            QPoint neighbors[] = {
                QPoint(pos.x(), pos.y() - 1), // up
                QPoint(pos.x(), pos.y() + 1), // down
                QPoint(pos.x() - 1, pos.y()), // left
                QPoint(pos.x() + 1, pos.y())  // right
            };

            bool isPerimeter = false;

            for (const QPoint& neighbor : neighbors)
            {
                // Check if the neighbor is within the image boundaries
                if (neighbor.x() >= 0 && neighbor.x() < segmentedImage.width() &&
                    neighbor.y() >= 0 && neighbor.y() < segmentedImage.height())
                {
                    // Check if the neighbor is not already in the segment coordinates
                    if (!segmentCoordinates.contains(neighbor))
                    {
                        // Check if the neighbor is a perimeter point
                        QColor neighborColor = segmentedImage.pixelColor(neighbor);
                        if (neighborColor != currentColor)
                        {
                            // Add the perimeter point to the perimeter coordinates
                            perimeterCoordinates.append(neighbor);
                            isPerimeter = true;
                        }

                        // Enqueue the neighbor for processing
                        queue.enqueue(neighbor);
                    }
                }
            }

            if (!isPerimeter)
            {
                // Add the current position to the perimeter coordinates
                perimeterCoordinates.append(pos);
            }
        }
    }
    qDebug() << "MainWindow::expandSegment, perimeterCoordinates size" << perimeterCoordinates.size();
    qDebug() << "MainWindow::expandSegment, segmentCoordinates size" << segmentCoordinates.size();
    return perimeterCoordinates;
}

void MainWindow::calculateSegmentProperties(const QVector<QPoint>& perimeterCoordinates)
{
    // Calculate the area of the segment (in % of the total image area)

    // Get image width and height, create an empty binary matrix
    int seg_cols = segmentedImage.width();
    int seg_rows = segmentedImage.height();
    int imageArea = seg_cols * seg_rows;

    // Calculate the area of the segment within the perimeter coordinates

    // Initialize the area to 0
    double segmentArea = 0.0;

    // Iterate over the perimeter coordinates
    int numPoints = perimeterCoordinates.size();
    for (int i = 0; i < numPoints; i++)
    {
        // Get the current and next point
        const QPoint& currentPoint = perimeterCoordinates[i];
        const QPoint& nextPoint = perimeterCoordinates[(i + 1) % numPoints]; // Wrap around to the first point

        // Calculate the cross product of the current and next point
        double crossProduct = currentPoint.x() * nextPoint.y() - nextPoint.x() * currentPoint.y();

        // Accumulate the cross product to the area
        segmentArea += crossProduct;
    }

    // Take the absolute value of the area and divide by 2
    segmentArea = std::abs(segmentArea) / 2.0;

    float segmentAreaPrc = (segmentArea / imageArea) * 100.0f;

    // Get the current time
    QTime currentTime = QTime::currentTime();

    // Convert the current time to QString format
    QString timeString = currentTime.toString("hh:mm:ss");

    // Create a string representation of the perimeter coordinates
    QString perimeterString;
    perimeterString.append("Start:" + timeString).append("\n");
    // Iterate over the coordinates and format them
    for (const QPoint& point : perimeterCoordinates)
    {
        QString pointString = QString("X=%1, Y=%2").arg(point.x()).arg(point.y());
        perimeterString.append(pointString).append("\n");
    }

    //int pointN = perimeterString.size();

    // Display the calculated area and perimeter coordinates
    //ui->pDark_3->setText(QString::number(seg_cols));
    ui->pDark_3->setText(QString::number(segmentAreaPrc));
    ui->pDark_4->setText(perimeterString);
}
