{
  gStyle->SetOptStat(0);

  gStyle->SetLabelSize(0.05,"xyz");
  gStyle->SetTitleSize(0.05,"xyz");

  gStyle->SetTitleOffset(1.05,"x");
  gStyle->SetTitleOffset(1.05,"y");

  gStyle->SetEndErrorSize(10);

  gStyle->SetStatY(0.5);
  gStyle->SetStatX(0.8);
  gStyle->SetStatW(0.15);
  gStyle->SetStatH(0.2);
  gStyle->SetOptFit(11);

  gROOT->ForceStyle();

  Double_t w(1600.);
  Double_t h(1000.);

  TCanvas canvas("canvas","TOT-Calibration",w,h);
  canvas.SetWindowSize(w + (w - canvas.GetWw()),h + (h - canvas.GetWh()));

  TTree *tree = new TTree("tree","TOT Calibration Data");

  int nlines = tree->ReadFile("TOTCalib.txt","uinj:tot:dtot");

  tree->Draw("uinj:tot:dtot","","goff");
  
  double uinj[nlines] = 0;
  double tot[nlines] = 0;
  double dtot[nlines] = 0;

  for(int i = 0; i < nlines; i++)
    {
      
      uinj[i] = tree->GetV1()[i];
      tot[i] = tree->GetV2()[i];
      dtot[i] = tree->GetV3()[i] / sqrt(4*256*256); //Correction for difference between error on mean and rms

    }

  delete tree;

  TGraphErrors graphCalibration(nlines,uinj,tot,0,dtot);

  graphCalibration.SetTitle("");
  graphCalibration.GetXaxis()->SetTitle("U_{inj} [mV]");
  graphCalibration.GetXaxis()->CenterTitle(true);
  graphCalibration.GetYaxis()->SetTitle("ToT [Clock cycles]");
  graphCalibration.GetYaxis()->CenterTitle(true);
  graphCalibration.SetLineWidth(3);
  graphCalibration.SetLineColor(kBlack);
  graphCalibration.SetMarkerStyle(kStar);
  graphCalibration.SetMarkerColor(kBlack);
  graphCalibration.SetMarkerSize(3);

  canvas.cd();

  graphCalibration.Draw("AP");

  TF1 functionCalibration("calibfct", " [0]*x + [1] - [2]/(x-[3])", 0 , 500);                                // thats the function that should fit the data....                                               
  functionCalibration->SetParameter(3, -100.00);
  functionCalibration->SetParameter(2, 205.735);
  functionCalibration->SetParameter(1, 23.5359);
  functionCalibration->SetParameter(0, 0.149194);

  functionCalibration->SetParLimits(3, -100, 0);

  functionCalibration->SetParName(3, "t");
  functionCalibration->SetParName(2, "c");
  functionCalibration->SetParName(1, "b");
  functionCalibration->SetParName(0, "a");

  functionCalibration->SetLineWidth(2);
  functionCalibration->SetLineColor(kRed);

  graphCalibration->Fit("calibfct", "ME+");
  
  canvas.Print("ToT-Calibration.pdf");
  canvas.Print("ToT-Calibration.eps");

}
