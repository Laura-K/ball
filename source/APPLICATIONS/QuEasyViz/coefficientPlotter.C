#include <coefficientPlotter.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_panner.h>

#include <QColor>

#include <BALL/QSAR/regressionModel.h>

using namespace BALL::QSAR;

namespace BALL
{
	namespace VIEW
	{

		CoefficientPlotter::CoefficientPlotter(ModelItem* model_item)
			: Plotter(model_item)
		{
			model_item_ = model_item;
			qwt_plot_->enableAxis(QwtPlot::yLeft);
			qwt_plot_->enableAxis(QwtPlot::yRight,0);
			plot(1);
			zoomer_ = new QwtPlotZoomer(qwt_plot_->canvas(),this);
			setWindowTitle("Coefficient Plotter");
			
			RegressionModel* model = (RegressionModel*)model_item_->model();
			int no_y = model->getTrainingResult()->cols();
			if(no_y>1)
			{
				activity_combobox_->addItem("All activities",-1);
				for(unsigned int i=0; i<no_y;i++)
				{
					String s = "Activity "+String(i);
					activity_combobox_->addItem(s.c_str(),i);
				}
				activity_combobox_->show();
			}
		}



		void CoefficientPlotter::plot(bool zoom)
		{
			qwt_plot_->clear();
			
			RegressionModel* model = (RegressionModel*)model_item_->model();
			const Eigen::MatrixXd* coefficient_matrix = model->getTrainingResult();
			
			if(coefficient_matrix->cols()==0)
			{
				std::cout << "Model must be trained before coefficients can be plotted!" << std::endl;
				return;
			}
			
			const vector<string>* feature_names;
			if(!model_item_->getRegistryEntry()->kernel)
			{
				feature_names = model->getDescriptorNames();
			}
			else
			{
				feature_names = model->getSubstanceNames();
			}
			
			bool show_stddev=0;
			const Eigen::MatrixXd* coeff_stddev = model->validation->getCoefficientStdErrors();
			if(coeff_stddev!=NULL && coeff_stddev->rows()==coefficient_matrix->rows() && coeff_stddev->cols()==coefficient_matrix->cols())
			{
				show_stddev=1;
			}
			
			double min_y=1e10;
			double max_y=-1e10;
			double min_x=1;
			double max_x=coefficient_matrix->rows();
			const unsigned int size = coefficient_matrix->rows();
			
			int start_matrixindex = selected_activity_+1;
			int end_matrixindex = selected_activity_+1;
			if(selected_activity_<0) // plot coefficients for all activities at once
			{
				start_matrixindex = 1;
				end_matrixindex = coefficient_matrix->cols();
			}
			for(int i=start_matrixindex; i<=end_matrixindex; i++)
			{
				QwtPlotCurve* curve_i = new QwtPlotCurve;
				double* x = new double[size];
				double* y = new double[size];
				
				for(unsigned int j=1; j<=size; j++)
				{
					QwtPlotMarker* marker= new QwtPlotMarker;
					marker->setSymbol(data_symbol);
					double coefficient_j = (*coefficient_matrix)(j,i);
					x[j-1] = j;
					y[j-1] = coefficient_j;	
					
					if(coefficient_j<min_y) min_y=coefficient_j;
					if(coefficient_j>max_y) max_y=coefficient_j;
					
					marker->setValue(j,coefficient_j);
					marker->attach(qwt_plot_); // attached object will be automatically deleted by QwtPlot
					
					if(show_data_labels)
					{
						QString s =(*feature_names)[j-1].c_str();
						QwtText label(s);
						label.setFont(data_label_font);
						marker->setLabel(label);
						marker->setLabelAlignment(data_label_alignment);
					}
					
					if(show_stddev)
					{
						double* sx = new double[2];
						double* sy = new double[2];
						double stddev = (*coeff_stddev)(j,i);
						sx[0]=j; sx[1]=j;
						sy[0]=coefficient_j-stddev; sy[1]=coefficient_j+stddev;
						QwtPlotCurve* error_bar = new QwtPlotCurve;
						error_bar->setData(sx,sy,2);
						QColor c(135,135,135); // grey
						QPen pen(c);
						error_bar->setPen(pen);
						error_bar->attach(qwt_plot_);
						delete [] sx; delete [] sy;
					}
				}
			
				curve_i->setData(x,y,size);
				delete [] x;
				delete [] y;
				QColor c(135,135,135);
				if(i==1)
				{ 
					c = QColor(190,10,10); // red
				}
				else if(i==2)
				{
					c = QColor(10,30,195); // blue
				}
				else if(i==3)
				{
					c = QColor(194,195,7); // yellow
				}
				else
				{
					c = QColor(rand()%255,rand()%255,rand()%255); // random color
				}
				QPen pen(c);
				curve_i->setPen(pen);
				curve_i->attach(qwt_plot_); // attached object will be automatically deleted by QwtPlot
			}
				
			
			QString s1 = "features";
			QString s2 = "coefficient values";
			qwt_plot_->setAxisTitle(0,s2);
			qwt_plot_->setAxisTitle(2,s1);
			
			double x_border=(max_x-min_x)*0.05;
			double y_border=(max_y-min_y)*0.05;
			min_x-=x_border; min_y-=y_border;
			max_x+=x_border; max_y+=y_border;
			
			QwtPlotCurve* zero_line = new QwtPlotCurve;
			double x[2]; x[0]=min_x; x[1]=max_x;
			double y[2]; y[0]=0; y[1]=0;
			zero_line->setData(x,y,2);
			QColor c(135,135,135); // grey
			QPen pen(c);
			zero_line->setPen(pen);
			zero_line->attach(qwt_plot_);
			
			if(zoom)
			{
				qwt_plot_->setAxisScale(QwtPlot::yLeft,min_y,max_y);
				qwt_plot_->setAxisScale(QwtPlot::xBottom,min_x,max_x);
			}
		}
	}
}
