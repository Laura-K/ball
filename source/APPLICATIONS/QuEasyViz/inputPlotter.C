#include <inputPlotter.h>

#include <qwt_plot_marker.h>
#include <qwt_plot_zoomer.h>

using namespace BALL::QSAR;

namespace BALL
{
	namespace VIEW
	{

		InputPlotter::InputPlotter(InputDataItem* item)
			: Plotter(item)
		{
			input_item_ = item;
			data_ = item->data();	
			sort_ = 0;
			sort_checkbox_ = new QCheckBox("sort",this);
			sort_checkbox_->setChecked(sort_);
			buttonsLayout_->addWidget(sort_checkbox_);
			connect(sort_checkbox_, SIGNAL(clicked()), this, SLOT(sortChangeState()));
			
			if(data_->getNoResponseVariables()>1)
			{
				for(unsigned int i=0; i<data_->getNoResponseVariables();i++)
				{
					String s = "Activity "+String(i);
					activity_combobox_->addItem(s.c_str(),i);
				}
				activity_combobox_->show();
			}
			
			plot(1);
			zoomer_ = new QwtPlotZoomer(qwt_plot_->canvas(),this);
			setWindowTitle("Input Data Plotter");
		}


		InputPlotter::~InputPlotter()
		{
			delete sort_checkbox_;	
		}

		void InputPlotter::plot(bool zoom)
		{
			qwt_plot_->clear();
			if(sort_)
			{
				plotSortedActivity(zoom);
			}
			else
			{
				plotActivity(zoom);
			}
		}


		// SLOT
		void InputPlotter::sortChangeState()
		{
			
			int a = sort_checkbox_->checkState();
			delete zoomer_;
			if(a==0) // unchecked
			{
				sort_ = 0;
				plot(1);
			}
			else if(a==2) // checked
			{
				sort_ = 1;
				plot(1);
			}
			zoomer_ = new QwtPlotZoomer(qwt_plot_->canvas(),this); // if not creating a new zoomer, zooming will not work correctly 
		}


		void InputPlotter::plotActivity(bool zoom)
		{
			const vector<string>* names = data_->getSubstanceNames();
			unsigned int no = data_->getNoResponseVariables();
			
			if(no==0)
			{
				std::cout<<"Activities must be read before they can be plotted!!"<<std::endl;
				return;
			}
			if(names->empty()) show_data_labels=0;
			
			double min_y=1e10;
			double max_y=-1e10;
			for(unsigned int i=0; i<data_->getNoSubstances();i++)
			{
				QwtPlotMarker* marker= new QwtPlotMarker;
				marker->setSymbol(data_symbol);
				vector<double>* y0 = data_->getActivity(i);
				double y = (*y0)[selected_activity_];
				delete y0;
				if(y<min_y) min_y=y;
				if(y>max_y) max_y=y;
				marker->setValue(i,y);
				marker->attach(qwt_plot_); // attached object will be automatically deleted by QwtPlot
				
				if(show_data_labels)
				{
					QString s =(*names)[i].c_str();
					QwtText label(s);
					label.setFont(data_label_font);
					marker->setLabel(label);
					marker->setLabelAlignment(data_label_alignment);
				}		
			}
			
			QString s1 = "compounds";
			QString s2 = "activity";
			qwt_plot_->setAxisTitle(0,s2);
			qwt_plot_->setAxisTitle(2,s1);
			
			double x_border=data_->getNoSubstances()*0.05;
			double y_border=(max_y-min_y)*0.05;
			double min_x=0-x_border; min_y-=y_border;
			double max_x=data_->getNoSubstances()+x_border; max_y+=y_border;
			
			if(zoom)
			{
				qwt_plot_->setAxisScale(0,min_y,max_y);
				qwt_plot_->setAxisScale(2,min_x,max_x);
			}
		}


		void InputPlotter::plotSortedActivity(bool zoom)
		{
			const vector<string>* names = data_->getSubstanceNames();
			unsigned int no = data_->getNoResponseVariables();
			
			if(no==0)
			{
				std::cout<<"Activities must be read before they can be plotted!!"<<std::endl;
				return;
			}
			if(names->empty()) show_data_labels=0;
			
			std::multiset<std::pair<double,unsigned int> > act;
			for(unsigned int i=0; i<data_->getNoSubstances();i++)
			{
				vector<double>* y0 = data_->getActivity(i);
				act.insert(std::make_pair((*y0)[selected_activity_],i));
				delete y0;
			}
				
			double min_y=1e10;
			double max_y=-1e10;
			std::multiset<std::pair<double, unsigned int> >::iterator a_it = act.begin();
			for(unsigned int i=0; i<data_->getNoSubstances();i++, ++a_it)
			{
				QwtPlotMarker* marker= new QwtPlotMarker;
				marker->setSymbol(data_symbol);
				const std::pair<double,unsigned int>& pair = *a_it;
				double y=pair.first;
				if(y<min_y) min_y=y;
				if(y>max_y) max_y=y;
				marker->setValue(i,y);
				marker->attach(qwt_plot_); // attached object will be automatically deleted by QwtPlot
				
				if(show_data_labels)
				{
					QString s =(*names)[pair.second].c_str();
					QwtText label(s);
					label.setFont(data_label_font);
					marker->setLabel(label);
					marker->setLabelAlignment(data_label_alignment);
				}		
			}
			
			QString s1 = "compounds sorted according to activity";
			QString s2 = "activity";
			qwt_plot_->setAxisTitle(0,s2);
			qwt_plot_->setAxisTitle(2,s1);
			
			double x_border=data_->getNoSubstances()*0.05;
			double y_border=(max_y-min_y)*0.05;
			double min_x=0-x_border; min_y-=y_border;
			double max_x=data_->getNoSubstances()+x_border; max_y+=y_border;
			
			if(zoom)
			{
				qwt_plot_->setAxisScale(0,min_y,max_y);
				qwt_plot_->setAxisScale(2,min_x,max_x);
			}
		}
	}
}
