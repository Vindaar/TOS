{
  gStyle->SetOptStat(0);

  gStyle->SetLabelSize(0.05,"xyz");
  gStyle->SetTitleSize(0.05,"xyz");

  gStyle->SetTitleOffset(1.05,"x");
  gStyle->SetTitleOffset(1.05,"y");

  gStyle->SetEndErrorSize(10);

  gROOT->ForceStyle();

  Double_t w(1600.);
  Double_t h(1000.);

  Double_t wSquare(720.);
  Double_t hSquare(600.);

  const Int_t NRGBs = 2;
  const Int_t NCont = 255;

  Double_t stops[NRGBs] = { 0.00, 1.00};
  Double_t red[NRGBs]   = { 0.00, 1.00};
  Double_t green[NRGBs] = { 0.00, 0.00};
  Double_t blue[NRGBs]  = { 1.00, 0.00};
  TColor::CreateGradientColorTable(NRGBs, stops, red, green, blue, NCont);
  gStyle->SetNumberContours(NCont);

  TCanvas canvasDistributions("distributions","Distributions",w,h);
  canvasDistributions.SetWindowSize(w + (w - canvasDistributions.GetWw()),h + (h - canvasDistributions.GetWh()));
  canvasDistributions.SetMargin(0.15,0.05,0.15,0.1);

  TCanvas canvasEqualisation("equalisation","Equalisation Bits",w,h);
  canvasEqualisation.SetWindowSize(w + (w - canvasEqualisation.GetWw()),h + (h - canvasEqualisation.GetWh()));
  canvasEqualisation.SetMargin(0.15,0.05,0.15,0.1);

  TCanvas canvasBits("bits","Bit Distribution",wSquare,hSquare);
  canvasBits.SetWindowSize(wSquare + (wSquare - canvasBits.GetWw()),hSquare + (hSquare - canvasBits.GetWh()));
  canvasBits.SetMargin(0.15,0.225,0.15,0.1); 

  TTree *tree = new TTree("treeone","Equalisation Result");

  int nlines = tree->ReadFile("thresholdMeans5.txt","x:y:min:max:bit:opt");

  Float_t x,y,min,max,bit,opt;

  tree->SetBranchAddress("x",&x);
  tree->SetBranchAddress("y",&y);
  tree->SetBranchAddress("min",&min);
  tree->SetBranchAddress("max",&max);
  tree->SetBranchAddress("bit",&bit);
  tree->SetBranchAddress("opt",&opt);

  TH1D minDistribution("mindistribution","",401,199.5,600.5);
  TH1D maxDistribution("maxdistribution","",401,199.5,600.5);
  TH1D optDistribution("optdistribution","",401,199.5,600.5);

  TH1D bitDistribution("bitdistribution","",16,-0.5,15.5);

  TH2D bitDistributionOnChip("bitdistributiononchip","",256,-0.5,255.5,256,-0.5,255.5);

  for(int i = 0; i < tree->GetEntries(); i++)
    {
      
      tree->GetEntry(i);

      minDistribution.Fill(min);
      maxDistribution.Fill(max);
      optDistribution.Fill(opt);

      bitdistribution.Fill(bit);

      bitDistributionOnChip.Fill(x,y,bit);

    }

  minDistribution.SetLineColor(kRed);
  minDistribution.SetFillColor(kRed);

  maxDistribution.SetLineColor(kBlue);
  maxDistribution.SetFillColor(kBlue);

  optDistribution.SetTitle("");
  optDistribution.SetLineColor(kBlack);
  optDistribution.SetFillColor(kBlack);

  bitDistribution.SetTitle("");
  bitDistribution.SetLineColor(kBlue);
  bitDistribution.SetFillColor(kBlue);

  bitDistributionOnChip.SetTitle("");

  canvasDistributions.cd();

  optDistribution.Draw();
  minDistribution.Draw("SAME");
  maxDistribution.Draw("SAME");
  optDistribution.Draw("SAME");
  
  TLegend thlLegend(0.55,0.7,0.85,0.85);
  thlLegend.SetFillColor(0);
  thlLegend.SetTextFont(42);
  thlLegend.SetTextSize(0.03);
  thlLegend.AddEntry(&optDistribution,"Equalisation bits optimised","f");
  thlLegend.AddEntry(&minDistribution,"Equalisation bits minimum","f");
  thlLegend.AddEntry(&maxDistribution,"Equalisation bits maximum","f");
  
  thlLegend.Draw();

  std::stringstream text;
  text << "rms_{opt} = " << optDistribution.GetRMS();

  TLatex widthAfterEqualisation(optDistribution.GetBinCenter(optDistribution.GetMaximumBin()) + 10,optDistribution.GetBinContent(optDistribution.GetMaximumBin()) /2.,text.str().c_str());
  widthAfterEqualisation.SetTextFont(42);
  widthAfterEqualisation.SetTextSize(0.05);

  widthAfterEqualisation.Draw();

  canvasEqualisation.cd();

  bitDistribution.Draw();

  canvasBits.cd();

  bitDistributionOnChip.Draw("colz");
  bitDistributionOnChip.GetZaxis()->SetRangeUser(-0.5,15.5);

  canvasDistributions.Print("THL-Distributions.pdf");
  canvasDistributions.Print("THL-Distributions.eps");

  canvasEqualisation.Print("Equalisation-Bits.pdf");
  canvasEqualisation.Print("Equalisation-Bits.eps");

  canvasBits.Print("Equalisation-Bits-Distrubition.pdf");
  canvasBits.Print("Equalisation-Bits-Distrubition.eps");

  delete tree;

}
