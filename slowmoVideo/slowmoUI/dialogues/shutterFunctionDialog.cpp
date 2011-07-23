#include "shutterFunctionDialog.h"
#include "ui_shutterFunctionDialog.h"
#include "project/shutterFunction_sV.h"
#include "project/shutterFunctionList_sV.h"
#include "project/project_sV.h"
#include <QtGui/QPainter>

QString ShutterFunctionDialog::emptyFunction("<None>");

ShutterFunctionDialog::ShutterFunctionDialog(Project_sV *project, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ShutterFunctionDialog),
    m_currentFunction(NULL)
{
    ui->setupUi(this);

    ui->lblcHeader->setText(ShutterFunction_sV::templateHeader);
    ui->lblcFooter->setText(ShutterFunction_sV::templateFooter);

    bool b = true;
    b &= connect(ui->bClose, SIGNAL(clicked()), this, SLOT(accept()));
//    b &= connect(ui->function, SIGNAL(textChanged()), this,  SLOT(slotUpdateCurve()));
//    b &= connect(ui->cbFunction, SIGNAL(currentIndexChanged(QString)), this, SLOT(slotUpdateCurrentFunction(QString)));
//    b &= connect(ui->cbFunction, SIGNAL(currentIndexChanged(QString)), this, SLOT(slotUpdateCurve()));
    b &= connect(ui->cbFunction, SIGNAL(currentIndexChanged(int)), this, SLOT(slotUpdateNode()));
    b &= connect(ui->cbFunction, SIGNAL(currentIndexChanged(int)), this, SLOT(slotLoadSelectedFunction()));
    b &= connect(ui->function, SIGNAL(textChanged()), this, SLOT(slotFunctionTextChanged()));
    b &= connect(ui->bAdd, SIGNAL(clicked()), this, SLOT(slotAddFunction()));
    b &= connect(ui->bRemove, SIGNAL(clicked()), this, SLOT(slotRemoveFunction()));

    b &= connect(ui->bPrevSegment, SIGNAL(clicked()), this, SLOT(slotPrevSegment()));
    b &= connect(ui->bNextSegment, SIGNAL(clicked()), this, SLOT(slotNextSegment()));
    Q_ASSERT(b);

    loadProject(project);
}

ShutterFunctionDialog::~ShutterFunctionDialog()
{
    delete ui;
}

void ShutterFunctionDialog::loadProject(Project_sV *project)
{
    qDebug() << "loadProject();";
    m_project = project;
    ui->cbFunction->blockSignals(true);
    ui->cbFunction->clear();
    ui->cbFunction->addItem(emptyFunction);
    for (int i = 0; i < project->shutterFunctions()->size(); i++) {
        ui->cbFunction->addItem(m_project->shutterFunctions()->at(i)->id());
    }
    ui->cbFunction->blockSignals(false);
    setSegment(0);
}

void ShutterFunctionDialog::paintEvent(QPaintEvent *e)
{
    QDialog::paintEvent(e);
    QPainter p(this);
    QImage img("res/shutterFunction.png");
    p.drawImage(ui->verticalLayout_code->contentsRect().topRight() - QPoint(img.width()+10, -10), img);
}

void ShutterFunctionDialog::slotNodesUpdated()
{
    qDebug() << "slotNodesUpdated();";
    if (m_segment+1 >= m_project->nodes()->size() && m_project->nodes()->size() >= 2) {
        setSegment(m_project->nodes()->size()-2);
    } else {
        setSegment(m_segment);
    }
}

void ShutterFunctionDialog::setSegment(int segment)
{
    qDebug() << "setSegment(" << segment << ");";
    m_segment = segment;
    ui->lblSegmentNumber->setText(QString("Segment %1 (total number: %2)")
                                  .arg(m_segment).arg(m_project->nodes()->size()-1));

    // Enable/disable buttons
    ui->bPrevSegment->setEnabled(m_segment > 0);
    ui->bNextSegment->setEnabled(m_segment+1 < m_project->nodes()->size()-1);


    // Update the curve parameters for this segment
    if (m_project->nodes()->size() >= 2) {
        const Node_sV *leftNode = &m_project->nodes()->at(m_segment);
        const Node_sV *rightNode = &m_project->nodes()->at(m_segment+1);
        ui->shutterCurve->updateValues(
                    leftNode->y(),
                    1.0/24 * (rightNode->y()-leftNode->y()) / (rightNode->x()-leftNode->x()) // dy = dx * /\y / /\x
                    );

        Q_ASSERT(m_segment+1 < m_project->nodes()->size());
    } else {
        qDebug() << "Less than 2 nodes!";
    }

    // Select function in the dropdown
    if (m_project->nodes()->size() >= 2) {
        const Node_sV *node = &m_project->nodes()->at(m_segment);
        QString id = node->shutterFunctionID();
        qDebug() << "Shutter function ID of node " << node << " is " << id;
        if (id.length() == 0) {
            ui->cbFunction->setCurrentIndex(ui->cbFunction->findText(emptyFunction));
        } else {
            int pos = ui->cbFunction->findText(id);
            Q_ASSERT(pos >= 0);
            ui->cbFunction->setCurrentIndex(pos);
        }
    }

    Q_ASSERT(m_segment >= 0);
}

void ShutterFunctionDialog::slotUpdateNode()
{
    qDebug() << "slotUpdateNode();";
    if (m_project->nodes()->size() >= 2) {
        Q_ASSERT(m_segment+1 < m_project->nodes()->size());
        QString id = ui->cbFunction->currentText();
        if (id == emptyFunction) {
            id = "";
        }
        Node_sV *node = &(*m_project->nodes())[m_segment];
        node->setShutterFunctionID(id);
        qDebug() << "Shutter function ID of node " << node << "set to " << id;
    }
}

void ShutterFunctionDialog::slotFunctionTextChanged()
{
    qDebug() << "slotUpdateFunctionCode();";
    if (m_currentFunction != NULL) {
        ui->function->setEnabled(true);
        m_currentFunction->updateFunction(ui->function->toPlainText());
        ui->shutterCurve->slotDisplayFunction(ui->function->toPlainText());
    }
    else {
        ui->function->setEnabled(false);
        ui->shutterCurve->slotDisplayFunction("return 0;");
    }
}

void ShutterFunctionDialog::slotLoadSelectedFunction()
{
    QString id = ui->cbFunction->currentText();
    if (id == emptyFunction) {
        id = "";
        m_currentFunction = NULL;
        ui->function->setPlainText("return 0;");
    } else {
        m_currentFunction = m_project->shutterFunctions()->function(id);
        ui->function->setPlainText(m_currentFunction->function());
    }


}

void ShutterFunctionDialog::slotAddFunction()
{
    ShutterFunction_sV *fun = m_project->shutterFunctions()->addFunction(ShutterFunction_sV(), true);
    if (fun != NULL) {
        ui->cbFunction->addItem(fun->id());
        ui->cbFunction->setCurrentIndex(ui->cbFunction->findText(fun->id()));
    } else {
        qDebug() << "Could not add new function.";
        Q_ASSERT(false);
    }
}

void ShutterFunctionDialog::slotRemoveFunction()
{
    if (ui->cbFunction->currentText() != emptyFunction) {
        bool ok = m_project->shutterFunctions()->removeFunction(ui->cbFunction->currentText());
        int index = ui->cbFunction->currentIndex();
        ui->cbFunction->setCurrentIndex(ui->cbFunction->findText(emptyFunction));
        ui->cbFunction->removeItem(index);
        Q_ASSERT(ok);
    }
}

void ShutterFunctionDialog::slotPrevSegment()
{
    qDebug() << "slotPrevSegment();";
    setSegment(m_segment-1);
}

void ShutterFunctionDialog::slotNextSegment()
{
    qDebug() << "slotNextSegment();";
    setSegment(m_segment+1);
}

