{

  gStyle->SetOptStat(0);

  gStyle->SetLabelSize(0.05,"xyz");
  gStyle->SetTitleSize(0.05,"xyz");

  gStyle->SetTitleOffset(1.05,"x");
  gStyle->SetTitleOffset(1.05,"y");

  gStyle->SetEndErrorSize(10);

  gStyle->SetStatY(0.85);
  gStyle->SetStatX(0.85);
  gStyle->SetStatW(0.15);
  gStyle->SetStatH(0.2);
  gStyle->SetOptFit(11);

  gROOT->ForceStyle();

  std::vector<int> voltages;
  
  voltages.push_back(0);
  voltages.push_back(20);
  voltages.push_back(25);
  voltages.push_back(30);
  voltages.push_back(35);
  voltages.push_back(40);
  voltages.push_back(50);
  voltages.push_back(60);
  voltages.push_back(100);

  double charge[voltages.size()] = 0;
  double mean[voltages.size()] = 0;
  double dmean[voltages.size()] = 0;
  double sigma[voltages.size()] = 0;
  double dsigma[voltages.size()] = 0;

  Double_t w(1600.);
  Double_t h(1000.);

  TCanvas canvas("canvas","THL-Calibration",w,h);
  canvas.SetWindowSize(w + (w - canvas.GetWw()),h + (h - canvas.GetWh()));

  for(int iVolt = 0; iVolt < voltages.size(); iVolt++)
    {

      charge[iVolt] = voltages[iVolt] * 50;

      std::stringstream infile;
      infile << "voltage_" << voltages[iVolt] << ".txt";

      TTree *tree = new TTree("tree","Input");
      int nlines = tree->ReadFile(infile.str().c_str(),"thl:counts");

      Float_t thl_branch, counts_branch;

      tree->SetBranchAddress("thl",&thl_branch);
      tree->SetBranchAddress("counts",&counts_branch);

      int thl[nlines] = 0;
      int counts[nlines] = 0;

      for(int i = 0; i < nlines; i++)
	{
	  tree->GetEntry(i);
	  
	  thl[i] = (int)thl_branch;
	  counts[i] = (int)counts_branch;
	}

      delete tree;

      TGraph graph(nlines,thl,counts);
      graph.SetTitle("");
      graph.GetXaxis()->SetTitle("THL");
      graph.GetXaxis()->CenterTitle(true);
      graph.GetYaxis()->SetTitle("Number of counts");
      graph.GetYaxis()->CenterTitle(true);
      graph.SetLineWidth(3);
      graph.SetLineColor(kBlack);
      graph.SetMarkerStyle(kStar);
      graph.SetMarkerColor(kBlack);
      graph.SetMarkerSize(1);

      canvas.cd();

      graph.Draw("AP");

      TF1 *function;

      int par_mean = 0;
      int par_amp = 0;
      int min = 0;
      int max = 0;
	  
      if(voltages[iVolt] == 0)
	{
	  function = new TF1("function","gaus(0)");

	  if(thl[0] > thl[nlines-1])
	    {
	      max = thl[0];
	      min = thl[nlines-1];
	    }
	  else
	    {
	      max = thl[nlines -1];
	      min = thl[0];
	    }

	  function->SetRange(min,max);
	  
	  int j = 0;
	  while(counts[j+1] >= par_amp && j < nlines)
	    {
	      j++;
	      par_amp = counts[j];
	      par_mean = thl[j];
	    }
	  
	  function->SetParameter(0,par_amp);
	  function->SetParameter(1,par_mean);
	  function->SetParameter(2,4.);
	}
      else
	{
	  function = new TF1("function","ROOT::Math::normal_cdf_c(x,[2],[1])*[0]");

	  if(thl[0] > thl[nlines-1])
	    {
	      max = thl[0];
	      int j = 0;
	      while(counts[j] <= 1000 && j < nlines)
		{
		  min = thl[j];		  
		  j++;
		}
	      j--;
	      while(counts[j] > 500)
		{
		  j--;
		  par_mean = thl[j];
		}
	    }
	  else
	    {
	      max = thl[nlines-1];
	      int j = nlines - 1;
	      while(counts[j] <= 1000 && j > -1)
		{
		  min = thl[j];
		  j--;
		}
	      j++;
	      while(counts[j] > 500)
		{
		  j++;
		  par_mean = thl[j];
		}
	    }

	  function->SetRange(min,max);

	  std::cout << par_mean << std::endl;

	  function->SetParameter(0,1000.);
	  function->SetParameter(1,par_mean);
	  function->SetParameter(2,5.);
	  
	}

      function->SetLineWidth(2);
      function->SetLineColor(kRed);

      function->SetParNames("A","#mu","#sigma");

      graph.Fit("function","MER+");

      mean[iVolt] = function->GetParameter(1);
      dmean[iVolt] = function->GetParError(1);
      sigma[iVolt] = function->GetParameter(2);
      dsigma[iVolt] = function->GetParError(2);

      std::stringstream outfilePDF;
      outfilePDF << "s-curve-" << voltages[iVolt] << "mV.pdf";
      std::stringstream outfileEPS;
      outfileEPS << "s-curve-" << voltages[iVolt] << "mV.eps";

      canvas.Print(outfilePDF.str().c_str());
      canvas.Print(outfileEPS.str().c_str());
      
      delete function;

    }

  TGraphErrors graphCalibration(voltages.size(),charge,mean,0,dmean);
  graphCalibration.SetTitle("");
  graphCalibration.GetXaxis()->SetTitle("Injected Charge [electrons]");
  graphCalibration.GetXaxis()->CenterTitle(true);
  graphCalibration.GetYaxis()->SetTitle("THL");
  graphCalibration.GetYaxis()->CenterTitle(true);
  graphCalibration.SetLineWidth(3);
  graphCalibration.SetLineColor(kBlack);
  graphCalibration.SetMarkerStyle(kStar);
  graphCalibration.SetMarkerColor(kBlack);
  graphCalibration.SetMarkerSize(3);

  canvas.cd();

  graphCalibration.Draw("AP");

  gStyle->SetStatX(0.5);

  TF1 functionCalibration("calibfct", "[0]+[1]*x");

  functionCalibration.SetRange(charge[0] , charge[voltages.size()-1]);

  functionCalibration->SetParName(1, "a");
  functionCalibration->SetParName(0, "b");

  functionCalibration->SetLineWidth(2);
  functionCalibration->SetLineColor(kRed);

  graphCalibration->Fit("calibfct", "MER+");

  canvas.Print("THL-Calibration.pdf");
  canvas.Print("THL-Calibration.eps");

  //print parameters
  for(int iVolt = 0; iVolt < voltages.size(); iVolt++)
    {
      std::cout << voltages[iVolt] << " mV\t";
      std::cout << charge[iVolt] << " e\t";
      std::cout << mean[iVolt] << " +/- " << dmean[iVolt] << "\t";
      std::cout << sigma[iVolt] << " +/- " << dsigma[iVolt] << std::endl;
    }

}
