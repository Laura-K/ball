// -*- Mode: C++; tab-width: 2; -*-
// vi: set ts=2:
//
// 

#include <BALL/QSAR/featureSelection.h>
using namespace BALL::QSAR;



FeatureSelection::FeatureSelection(Model& m)
{
	model_ = &m;
	weights_ = NULL;
	quality_increase_cutoff_ = 0.001;
}

FeatureSelection::FeatureSelection(KernelModel& km)
{
	model_ = &km;
	weights_ = &km.kernel->weights_;
	quality_increase_cutoff_ = 0.001;
}

FeatureSelection::~FeatureSelection()
{
}

void FeatureSelection::setModel(Model& m)
{
	model_ = &m;
	weights_ = NULL;
}
	
void FeatureSelection::setModel(KernelModel& km)
{
	model_ = &km;
	weights_ = &km.kernel->weights_;
}


void FeatureSelection::forwardSelection(int k, bool optPar)
{
	forward(0,k,optPar);
}


void FeatureSelection::selectStat(int s)
{
	if(model_==0) return;
	model_->model_val->selectStat(s);
}


void FeatureSelection::setQualityIncreaseCutoff(double& d)
{
	if(d<-1 || d>1)
	{
		throw Exception::FeatureSelectionParameterError(__FILE__,__LINE__,"The quality increase cutoff value for feature selection must be between -1 and 1 !");
	}	
	quality_increase_cutoff_ = d;
}
		
		
void FeatureSelection::forward(bool stepwise, int k, bool optPar)
{
	unsigned int columns=model_->data->descriptor_matrix_.size();
	unsigned int lines=model_->data->descriptor_matrix_[0].size();
	SortedList<unsigned int>* irrelevantDescriptors = findIrrelevantDescriptors();
	
	// ------ Q2 value for regression using all descriptors  --- //
	double q2_allDes=0;
	SortedList<unsigned int> old_descr=model_->descriptor_IDs_;
	int col=model_->descriptor_matrix_.Ncols();
	if(model_->descriptor_IDs_.size()!=0)
	{
		col=model_->descriptor_IDs_.size();
	}

	if(lines<1000 && col<1000 && model_->type_!="ALL" && model_->type_!="SVR" && model_->type_!="SVC" && (col<model_->data->descriptor_matrix_[0].size()*(((double)k-1)/k) || model_->type_!="MLR"))
	{
		if(!optPar || !model_->optimizeParameters(k))
		{
			model_->model_val->crossValidation(k);
		}
		q2_allDes=model_->model_val->getCVRes();
	}
	// --------------------------------------------------------- //
	
	Vector<double> oldWeights;
	if(weights_!=NULL)
	{	
		oldWeights=*weights_;
	}

	model_->descriptor_IDs_.clear();
	double old_q2=0;
	// do while there is an increase of Q^2 (and no of columns < no of lines)
	int crossValidation_lines = (int)(((double)lines/k)*(k-1));
	
	SortedList<unsigned int>::Iterator des_it;
	SortedList<unsigned int>::Iterator irr_it;

	for(int d=0; d<crossValidation_lines; d++)
	{
		int best_col=0; 
		double best_q2=0;
		des_it = model_->descriptor_IDs_.begin();
		irr_it = irrelevantDescriptors->begin();
	
		// find the descriptor that leads to the largest increase of Q^2
		for(unsigned int i=0; i<columns; i++)
		{	
			// do not insert a descriptor more than one time and do not use irrelevant descriptors
			bool c=0;
			if(*des_it==i) 
			{
				des_it++;
				c=1;
			}
			if(*irr_it==i)
			{
				irr_it++;
				c=1;
			}
			if(c==1)
			{
				continue;
			}
			
			model_->descriptor_IDs_.insert(des_it,i);
			
			if(weights_!=NULL && weights_->getSize()>0)
			{
				updateWeights(old_descr,model_->descriptor_IDs_,oldWeights);
			}
			
			try
			{
				if(!optPar || !model_->optimizeParameters(k))
				{
					model_->model_val->crossValidation(k,0);
				}
			}
			catch(Exception::NoPCAVariance e)
			{   
				// if selected descriptor(s) yield no PCA eigenvectors, ignore this combination of descriptors!
				model_->descriptor_IDs_.deleteLastInsertion();
				continue;
			}
			if(model_->model_val->getCVRes() > best_q2)
			{
				best_q2=model_->model_val->getCVRes();
				best_col=i;
			}
			model_->descriptor_IDs_.deleteLastInsertion();
		}
	
		// if Q^2 with the new descriptor is not larger than before, it is not selected and feature selection is stopped.
		if (best_q2 <= old_q2+quality_increase_cutoff_ || model_->descriptor_IDs_.size()==lines-1) 
		{
			break;
		}
	
		// insert no of best descriptor into the list, if accuracy was increased by it
		if(best_q2 > old_q2)
		{	
			model_->descriptor_IDs_.insert(best_col);
			old_q2=best_q2;
			if(stepwise)
			{
				backwardSelection(k,optPar); /// => STEPWISE !!
				old_q2=model_->model_val->getCVRes();
			}
		}		
	}
	
	delete irrelevantDescriptors;
	
	// if feature selection leads to no increase of Q^2, use old descriptors and weights_.
	// Also use old descriptors, if no better combination of descriptors could be found by this feature selection run, since features not selected by the previous feature selection may not be used!!
	if(old_q2<q2_allDes || model_->descriptor_IDs_.size()==0) 
	{	
		model_->model_val->setCVRes(q2_allDes);
		model_->descriptor_IDs_=old_descr;
		if(weights_!=NULL && weights_->getSize()>0)
		{
			*weights_=oldWeights;
		}
	}
	else 
	{	
		model_->model_val->setCVRes(old_q2);
		if(weights_!=NULL && weights_->getSize()>0) 
		{
			updateWeights(old_descr,model_->descriptor_IDs_,oldWeights);
		}
	}
	
	// optimize parameters for the best set of descriptors 
	if (optPar)
	{
		model_->optimizeParameters(k);
	}
	
	// read all information about the selected descriptors (their names, mean, stddev)
	model_->readDescriptorInformation();
}



void FeatureSelection::backwardSelection(int k, bool optPar)
{
	int columns=model_->data->descriptor_matrix_.size();

	SortedList<unsigned int>* irrelevantDescriptors = findIrrelevantDescriptors();
	SortedList<unsigned int> old_descr=model_->descriptor_IDs_;
	
	// ------ Q2 value for regression using all descriptors  --- //
	double q2_allDes=0;
	int col=model_->descriptor_matrix_.Ncols();
	if(model_->descriptor_IDs_.size()!=0)
	{
		col=model_->descriptor_IDs_.size();
	}
	
	if(col<model_->descriptor_matrix_.Nrows()*(((double)k-1)/k) || model_->type_!="MLR")
	{
		if(!optPar || !model_->optimizeParameters(k))
		{
			model_->model_val->crossValidation(k);
		}
		q2_allDes=model_->model_val->getCVRes();
	}
	// --------------------------------------------------------- //
	
	Vector<double> oldWeights;
	if(weights_!=NULL)
	{	
		oldWeights=*weights_;
	}
	// if no feature selection has already been done, start backward selection with all descriptors
	if(model_->descriptor_IDs_.empty())
	{
		for (int i=0; i<columns;i++)
		{
			model_->descriptor_IDs_.push_back(i);
		}
	}
	double old_q2=q2_allDes;
	SortedList<unsigned int>::Iterator des_it;
	SortedList<unsigned int>::Iterator irr_it;
	
	// the quality of the model must always be larger than this value;
	// if a negative quality_increase_cutoff_ is specified, minimal reduction of
	// the model's predictive quality by removing descriptors is allowed this way 
	double min_quality_threshold = q2_allDes;
	if(quality_increase_cutoff_<0 && min_quality_threshold+quality_increase_cutoff_>=0) 
	{
		min_quality_threshold+=quality_increase_cutoff_;
	}
	
	// do while there is an increase of Q^2 
	while(model_->descriptor_IDs_.size()>1)
	{	
		int best_col=0; 
		double best_q2=0;
		des_it = model_->descriptor_IDs_.begin();
		irr_it = irrelevantDescriptors->begin();
		
		// find the descriptor whose removal leads to the largest increase of Q^2
		while(des_it!=model_->descriptor_IDs_.end())
		{	
			unsigned int i = *des_it;
		
			// do not use empty descriptors
			if(*irr_it==i)
			{
				des_it++;
				irr_it++;
				continue;
			}
			
			//SortedList<int>::Iterator tmp=des_it; tmp++;
			model_->descriptor_IDs_.erase(des_it); //erase element and move des_it to next element
			//des_it=tmp;
			
			
			if(weights_!=NULL && weights_->getSize()>0)
			{
				updateWeights(old_descr,model_->descriptor_IDs_,oldWeights);
			}

			try
			{
				if(!optPar || !model_->optimizeParameters(k))
				{
					model_->model_val->crossValidation(k,0);
				}
			}
			catch(Exception::NoPCAVariance e)
			{   // if selected descriptor(s) yield no PCA eigenvectors, ignore this combination of descriptors!
				model_->descriptor_IDs_.deleteLastInsertion();
				continue;
			}
			
			if(model_->model_val->getCVRes() > best_q2)
			{
				best_q2=model_->model_val->getCVRes();
				best_col=i;
			}
		
			model_->descriptor_IDs_.insert(des_it,i);
		}
	
		// if Q^2 is not larger than before, no descriptor is removed and feature selection is stopped.
		if (best_q2<=old_q2+quality_increase_cutoff_ || best_q2<min_quality_threshold) 
		{
			break;
		}
		//if(best_q2 > old_q2)
		else
		{
			model_->descriptor_IDs_.remove(best_col);
			//cout << "  removed no"<<best_col<<endl;
			old_q2=best_q2;
		}
	}

	delete irrelevantDescriptors;
	
	// if feature selection leads significant decrease of Q^2, use old descriptors and weights_
	if(old_q2<min_quality_threshold) 
	{	
		model_->model_val->setCVRes(q2_allDes);
		model_->descriptor_IDs_=old_descr;
		if(weights_!=NULL && weights_->getSize()>0)
		{
			*weights_=oldWeights;
		}
	}
	else
	{	
		model_->model_val->setCVRes(old_q2);
		if(weights_!=NULL && weights_->getSize()>0) 
		{
			updateWeights(old_descr,model_->descriptor_IDs_,oldWeights);
		}
	}
	
	// optimize parameters for the best set of descriptors 
	if (optPar)
	{
		model_->optimizeParameters(k);
	}
	
	// read all information about the selected descriptors (their names, mean, stddev)
	model_->readDescriptorInformation();
}


void FeatureSelection::stepwiseSelection(int k, bool optPar)
{
	
	forward(1,k,optPar);
// 	int size = model_->descriptor_IDs_.size();
// 	if(size==0)
// 	{
// 		size = model_->data->descriptor_matrix_.size();
// 	}
// 	
// 	for(int i=0; i<100; i++) // do until the number of features decreases
// 	{
// 		if(i%2==0)
// 		{
// 			forwardSelection(k, opt); //cout <<"size after forwSel = "<<model_->descriptor_IDs_.size()<<endl;
// 		}
// 		else
// 		{
// 			backwardSelection(k, opt); //cout <<"size after backwSel = "<<model_->descriptor_IDs_.size()<<endl;
// 		}
// 		
// 		if(model_->descriptor_IDs_.size()<size)
// 		{
// 			size=model_->descriptor_IDs_.size();
// 		}
// 		else
// 		{
// 			break;
// 		}
// 	}
	
}


void FeatureSelection::twinScan(int k, bool optPar)
{
	unsigned int columns=model_->data->descriptor_matrix_.size();
	unsigned int lines=model_->data->descriptor_matrix_[0].size();
	SortedList<unsigned int>* irrelevantDescriptors = findIrrelevantDescriptors();
	
	// ------ Q2 value for regression using all descriptors  --- //
	double q2_allDes=0;
	SortedList<unsigned int> old_descr=model_->descriptor_IDs_;
	int col=model_->descriptor_matrix_.Ncols();
	if(model_->descriptor_IDs_.size()!=0)
	{
		col=model_->descriptor_IDs_.size();
	}

	if(lines<1000 && col<1000 && model_->type_!="ALL" && model_->type_!="SVR" && model_->type_!="SVC" && (col<model_->data->descriptor_matrix_[0].size()*(((double)k-1)/k) || model_->type_!="MLR"))
	{
		if(!optPar || !model_->optimizeParameters(k))
		{
			model_->model_val->crossValidation(k);
		}
		q2_allDes=model_->model_val->getCVRes();
	}
	// --------------------------------------------------------- //
	
	Vector<double> oldWeights;
	if(weights_!=NULL)
	{	
		oldWeights=*weights_;
	}

	model_->descriptor_IDs_.clear();
	// do while there is an increase of Q^2 (and no of columns < no of lines)
	// int crossValidation_lines = (int)(((double)lines/k)*(k-1));
	
	SortedList<unsigned int>::Iterator des_it;
	SortedList<unsigned int>::Iterator irr_it;

	/// find the best feature to be selected first
	int best_col=0; 
	double best_q2=0;
	des_it = model_->descriptor_IDs_.begin();
	irr_it = irrelevantDescriptors->begin();
	
	SortedList<pair<double,uint> > potential_descriptors; // sorted descendingly by their increase of prediction quality during the first scan
	
	SortedList<pair<double,uint> >::Iterator it_to_best=potential_descriptors.end();
	
	for(uint i=0; i<columns; i++)
	{	
		// do not insert a descriptor more than one time and do not use irrelevant descriptors
		bool c=0;
		if(*des_it==i) 
		{
			des_it++;
			c=1;
		}
		if(*irr_it==i)
		{
			irr_it++;
			c=1;
		}
		if(c==1)
		{
			continue;
		}
		
		model_->descriptor_IDs_.insert(des_it,i);
		
		if(weights_!=NULL && weights_->getSize()>0)
		{
			updateWeights(old_descr,model_->descriptor_IDs_,oldWeights);
		}
		
		try
		{
			if(!optPar || !model_->optimizeParameters(k))
			{
				model_->model_val->crossValidation(k,0);
			}
		}
		catch(Exception::NoPCAVariance e)
		{   
			// if selected descriptor(s) yield no PCA eigenvectors, ignore this combination of descriptors!
			model_->descriptor_IDs_.deleteLastInsertion();
			continue;
		}
		
		SortedList<pair<double,uint> >::Iterator it = potential_descriptors.insert(make_pair(1-model_->model_val->getCVRes(),i));
		
		if(model_->model_val->getCVRes() > best_q2)
		{
			best_q2=model_->model_val->getCVRes();
			best_col=i;
			it_to_best = it;
		}
		model_->descriptor_IDs_.deleteLastInsertion();
	}
	
	model_->descriptor_IDs_.insert(best_col);
	potential_descriptors.erase(it_to_best);
	
	/// now check ONCE for each remaining (non-empty) descriptor, whether it can increase the prediction quality
 	double old_q2=best_q2;
	
	potential_descriptors.front();
	while(potential_descriptors.hasNext())
	{
		uint i = potential_descriptors.next().second;
		model_->descriptor_IDs_.insert(i);
	
		try
		{
			if(!optPar || !model_->optimizeParameters(k))
			{
				model_->model_val->crossValidation(k,0);
			}
		}
		catch(Exception::NoPCAVariance e)
		{ 
			// if selected descriptor(s) yield no PCA eigenvectors, ignore this combination of descriptors!
			model_->descriptor_IDs_.deleteLastInsertion();
			continue;
		}
		
		if(model_->model_val->getCVRes()<=old_q2+quality_increase_cutoff_) // delete descriptor if no quality increase
		{
			model_->descriptor_IDs_.deleteLastInsertion();
		}
		else
		{
			old_q2=model_->model_val->getCVRes();
		}
	}
	
	delete irrelevantDescriptors;
	
	
	// if feature selection leads to no increase of Q^2, use old descriptors and weights_.
	// Also use old descriptors, if no better combination of descriptors could be found by this feature selection run, since features not selected by the previous feature selection may not be used!!
	if(old_q2<q2_allDes || model_->descriptor_IDs_.size()==0) 
	{	
		model_->model_val->setCVRes(q2_allDes);
		model_->descriptor_IDs_=old_descr;
		if(weights_!=NULL && weights_->getSize()>0)
		{
			*weights_=oldWeights;
		}
	}
	else 
	{	
		model_->model_val->setCVRes(old_q2);
		if(weights_!=NULL && weights_->getSize()>0) 
		{
			updateWeights(old_descr,model_->descriptor_IDs_,oldWeights);
		}
	}
	
	// optimize parameters for the best set of descriptors 
	if (optPar)
	{
		model_->optimizeParameters(k);
	}
	
	// read all information about the selected descriptors (their names, mean, stddev)
	model_->readDescriptorInformation();
}


/*
void FeatureSelection::forwardSelection(bool optPar)
{
	int columns=model_->data->descriptor_matrix_.size();
	int lines=model_->data->descriptor_matrix_[0].size();

	SortedList<int>* irrelevantDescriptors = findIrrelevantDescriptors();
	
	// ------ Q2 value for regression using all descriptors  --- //
	double q2_allDes=0;
	SortedList<int> old_descr=model_->descriptor_IDs_;
	
	if(model_->descriptor_matrix_.Ncols()<model_->descriptor_matrix_.Nrows()*3/4 || model_->type_!="MLR")
	{
		if(!optPar || !model_->optimizeParameters())
		{
			model_->model_val->crossValidation(4);
		}
		q2_allDes=model_->model_val->getCVRes();
	}
	// --------------------------------------------------------- //
	
	RowVector oldWeights;
	if(weights_!=NULL)
	{	
		oldWeights=*weights_;
	}
	
	model_->descriptor_IDs_.clear();
	
	double old_q2=0;
	// do while there is an increase of Q^2 (and no of columns < no of lines)
	int crossValidation_lines = (lines/4)*3-1;
	
	for(int d=0; d<crossValidation_lines; d++)
	{	
		int best_col=0; 
		double best_q2=0;
		
		// find the descriptor that leads to the largest increase of Q^2
		for(int i=0; i<columns; i++)
		{	
			// do not insert a descriptor more than one time and do not use irrelevant descriptors
			if ( model_->descriptor_IDs_.contains(i) || irrelevantDescriptors->contains(i) )
			{
				continue;
			}
			
			model_->descriptor_IDs_.insert(i);
			
			if(weights_!=NULL && weights_->Ncols()>0)
			{
				updateWeights(old_descr,model_->descriptor_IDs_,oldWeights);
			}

			if(!optPar || !model_->optimizeParameters())
			{
				model_->model_val->crossValidation(4);
			}
			
			if(model_->model_val->getCVRes() > best_q2)
			{
				best_q2=model_->model_val->getCVRes();
				best_col=i;
			}
			model_->descriptor_IDs_.deleteLastInsertion();
		}
		
		// if Q^2 with the new descriptor is not larger than before, it is not selected and feature selection is stopped.
		if (best_q2 <= old_q2 || model_->descriptor_IDs_.size()==lines-1) 
		{
			break;
		}

		// insert no of best descriptor into the list, if accuracy was increased by it
		if(best_q2 > old_q2)
		{
			model_->descriptor_IDs_.insert(best_col);
			old_q2=best_q2;
		}
	}
	
	delete irrelevantDescriptors;
	
	// if feature selection leads to no increase of Q^2, use old descriptors and weights_
	if(old_q2<q2_allDes) 
	{	
		model_->model_val->setCVRes(q2_allDes);
		model_->descriptor_IDs_=old_descr;
		if(weights_!=NULL && weights_->Ncols()>0)
		{
			*weights_=oldWeights;
		}
	}
	else
	{
		model_->model_val->setCVRes(old_q2);
		if(weights_!=NULL && weights_->Ncols()>0) 
		{
			updateWeights(old_descr,model_->descriptor_IDs_,oldWeights);
		}
	}
	
	// optimize parameters for the best set of descriptors 
	if (optPar)
	{
		model_->optimizeParameters();
	}	

}
*/


SortedList<unsigned int>* FeatureSelection::findIrrelevantDescriptors()
{
	SortedList<unsigned int>* irrelevantDescriptors=new SortedList<unsigned int>();
	for (int i=0; i<(int)model_->data->descriptor_matrix_.size();i++)
	{
		bool empty=1;
		for(int j=0; j<(int)model_->data->descriptor_matrix_[i].size();j++)
		{
			if(model_->data->descriptor_matrix_[i][j]!=model_->data->descriptor_matrix_[i][0])
			{
				empty=0;
				break;
			}
		}
		// if column is empty or not already selected, add it to the list
		if(empty || (!model_->descriptor_IDs_.empty() && !model_->descriptor_IDs_.contains(i)) )
		{
			irrelevantDescriptors->push_back(i);
		}
	}
	return irrelevantDescriptors;
}


void FeatureSelection::removeEmptyDescriptors()
{
	if(model_->descriptor_IDs_.size()!=0)  // if a feature selection method has already been applied
	{
		model_->descriptor_IDs_.front();
		while(model_->descriptor_IDs_.hasNext())
		{
			int col=model_->descriptor_IDs_.next();
			bool empty=1;
			for(int j=0; j<(int)model_->data->descriptor_matrix_[col].size();j++)
			{
				if(model_->data->descriptor_matrix_[col][j]!=0)
				{
					empty=0;
					break;
				}
			}
			if(empty)
			{
				model_->descriptor_IDs_.erase(col);
			}
		}
	}
	else
	{
		for(int i=0; i<(int)model_->data->descriptor_matrix_.size();i++)
		{
			bool empty=1;
			for(int j=0; j<(int)model_->data->descriptor_matrix_[i].size();j++)
			{
				if(model_->data->descriptor_matrix_[i][j]!=0)
				{
					empty=0;
					break;
				}
			}
			if(!empty)
			{
				model_->descriptor_IDs_.insert(i);
			}
		}
	}
	
	// read all information about the selected descriptors (their names, mean, stddev)
	model_->readDescriptorInformation();
}



void FeatureSelection::removeHighlyCorrelatedFeatures(double& cor_threshold)
{	
	removeEmptyDescriptors(); // -> descriptor_IDs now contains the IDs of all non-empty descriptors

	vector<double> stddev(model_->data->getNoDescriptors(),1);
	vector<double> mean(model_->data->getNoDescriptors(),0);
	
	// if data has not been centered, calculate mean and stddev of each feature
	if(model_->data->descriptor_transformations_.size()==0)
	{
		for(uint i=0; i<mean.size();i++)
		{
			mean[i] = Statistics::getMean(model_->data->descriptor_matrix_[i]);
		}		
		for(uint i=0; i<stddev.size();i++)
		{
			stddev[i] = Statistics::getStddev(model_->data->descriptor_matrix_[i], mean[i]);
		}
	}
		
	double abs_cor_threshold = abs(cor_threshold);
	
	for(SortedList<uint>::iterator it1 = model_->descriptor_IDs_.begin(); it1!=model_->descriptor_IDs_.end(); it1++)
	{	
		for(SortedList<uint>::iterator it2 = model_->descriptor_IDs_.begin(); it2!=model_->descriptor_IDs_.end(); it2++)
		{
			if(*it1==*it2) continue;
			
			double covar = Statistics::getCovariance(model_->data->descriptor_matrix_[*it1], model_->data->descriptor_matrix_[*it2],mean[*it1],mean[*it2]);
			
			double abs_cor = abs(covar/(stddev[*it1]*stddev[*it2]));
			
			if(abs_cor>abs_cor_threshold)
			{
				model_->descriptor_IDs_.erase(it2); // element at this position is deleted and it2 is automagically set to next element
				it2--; // make sure also to check the next feature in the next iteration...
			}
		}
	}
}


void FeatureSelection::removeLowResponseCorrelation(double& min_correlation)
{
	removeEmptyDescriptors(); // -> descriptor_IDs now contains the IDs of all non-empty descriptors
	
	vector<double> desc_stddev(model_->data->getNoDescriptors(),1);
	vector<double> desc_mean(model_->data->getNoDescriptors(),0);
	
	uint no_y=model_->data->Y_.size();
	vector<double> y_stddev(no_y,1);
	vector<double> y_mean(no_y,0);
	
	// if data has not been centered, calculate mean and stddev of each feature
	if(model_->data->descriptor_transformations_.size()==0)
	{
		for(uint i=0; i<desc_mean.size();i++)
		{
			desc_mean[i] = Statistics::getMean(model_->data->descriptor_matrix_[i]);
		}		
		for(uint i=0; i<desc_stddev.size();i++)
		{
			desc_stddev[i] = Statistics::getStddev(model_->data->descriptor_matrix_[i], desc_mean[i]);
		}
	}
	// if response variables have not been centered, calculate their mean and stddev
	if(model_->data->y_transformations_.size()==0)
	{
		for(uint i=0; i<y_mean.size();i++)
		{
			y_mean[i] = Statistics::getMean(model_->data->Y_[i]);
		}		
		for(uint i=0; i<y_stddev.size();i++)
		{
			y_stddev[i] = Statistics::getStddev(model_->data->Y_[i], y_mean[i]);
		}
	}
		
	double abs_cor_threshold = abs(min_correlation);
	
	/// check correlation of each feature with each response variable
	for(SortedList<uint>::iterator it = model_->descriptor_IDs_.begin(); it!=model_->descriptor_IDs_.end(); it++)
	{	
		double max_abs_cor = 0;
		
		for(uint i=0;i<no_y;i++)
		{
			double covar = Statistics::getCovariance(model_->data->descriptor_matrix_[*it], model_->data->Y_[i],desc_mean[*it],y_mean[i]);
			
			double abs_cor = abs(covar/(desc_stddev[*it]*y_stddev[i]));
			if(abs_cor>max_abs_cor) max_abs_cor=abs_cor;
		}
		
		if(max_abs_cor<abs_cor_threshold)
		{
			model_->descriptor_IDs_.erase(it); // element at this position is deleted and 'it' is automagically set to next element
			it--; // make sure also to check the next feature in the next iteration...
		}
	}	
}



void FeatureSelection::implicitSelection(LinearModel& lm, int act, double d)
{
	SortedList<unsigned int> newIDs;
	const Matrix<double>* training_result=lm.getTrainingResult();
	const Matrix<double>* coeff_errors=lm.validation->getCoefficientStdErrors();
	
	if(coeff_errors->Nrows()==0)
	{
		throw Exception::InconsistentUsage(__FILE__,__LINE__,"The standard deviations of the coefficients of the given LinearModel must be calculated before implicit feature selection can be done!");
	}
	
	
	if(!lm.descriptor_IDs_.empty())
	{
		lm.descriptor_IDs_.front(); 
		for(int i=1; i<=training_result->Nrows() && lm.descriptor_IDs_.hasNext(); i++)
		{	
			int id=lm.descriptor_IDs_.next();
			// consider only those descriptors that are already part of BOTH models AND that have a coefficient outside of 0 +/- stddev
			if (  (model_->descriptor_IDs_.empty() || model_->descriptor_IDs_.contains(id))
					&&  ((*training_result)(i,act)>(*coeff_errors)(i,act)*d || (*training_result)(i,act)< -(*coeff_errors)(i,act)*d) )
			{
				newIDs.push_back(id); //cout<<"  added "<<id<<endl;
			}
		
		}
	}
	else
	{
		for(int i=0; i<training_result->Nrows();i++)
		{		
			// consider only those descriptors that are already part of BOTH models AND that have a coefficient outside of 0 +/- stddev
			if (  (model_->descriptor_IDs_.empty() || model_->descriptor_IDs_.contains(i))
						    &&  ((*training_result)(i+1,act)>(*coeff_errors)(i+1,act)*d || (*training_result)(i+1,act)< -(*coeff_errors)(i+1,act)*d) )
			{
				newIDs.push_back(i); //cout<<"  added "<<i-1<<endl;
			}
		
		}	
	}
			
	if(weights_!=NULL && weights_->getSize()>0)
	{
		Vector<double> oldWeights = *weights_;
		updateWeights(model_->descriptor_IDs_,newIDs,oldWeights);
	}
	model_->descriptor_IDs_=newIDs;
	
	// read all information about the selected descriptors (their names, mean, stddev)
	model_->readDescriptorInformation();
}


void FeatureSelection::updateWeights(SortedList<unsigned int>& oldDescIDs, SortedList<unsigned int>& newDescIDs, Vector<double>& oldWeights)
{
	weights_->resize(newDescIDs.size());
	
	SortedList<unsigned int>::Iterator it = newDescIDs.begin();
	int posNew=1;
	
	// if a previous feature selection was done, find the position of all relevant weights_ by getting the position of each relevant descriptor within the old list
	if(oldDescIDs.size()!=0)
	{
		while(it!=newDescIDs.end())
		{
			unsigned int value=*it;
			SortedList<unsigned int>::Iterator it1 = oldDescIDs.begin();
			int posOld=1;
			// search position of current descriptor ID in oldDescIDs
			while(*it1<value && it1!=oldDescIDs.end())
			{
				it1++;
				posOld++;
			}
			if(*it1==value)
			{
				(*weights_)(posNew)=oldWeights(posOld);
				posNew++;
			}
			it++;
		}
	}
	
	// if no previous feature selection was done, copy relevant weights_ directly
	else
	{
		while(it!=newDescIDs.end())
		{	
			(*weights_)(posNew)=oldWeights(*it+1);
			posNew++;
			it++;
		}
	}			
}

