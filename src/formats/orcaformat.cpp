/**********************************************************************
Copyright (C) 2001-2006 by Geoffrey R. Hutchison
Some portions Copyright (C) 2004 by Chris Morley
Some portions Copyright (C) 2009 by Michael Banck
Some portions Copyright (C) 2014 by Dagmar Lenk

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
***********************************************************************/
#include <openbabel/babelconfig.h>

#include <openbabel/obmolecformat.h>
#ifdef _MSC_VER
#include <regex>
#else
#include <regex.h>
#endif

#include <iomanip>
#define _eV_kcal        23.0605             // eV into kcal/mol
#define _au_eV          27.2113834          // change of a.u. to eV

#define KCAL_TO_KJ	4.1868


#define notFound string::npos
using namespace std;
namespace OpenBabel
{

  class OrcaOutputFormat : public OBMoleculeFormat
  {
  public:
    //Register this format type ID
    OrcaOutputFormat()
    {
      OBConversion::RegisterFormat("orca",this);
    }

    virtual const char* Description() //required
    {
      return
        "Orca output format\n"
        "Read Options e.g. -as\n"
        " s  Output single bonds only\n"
        " b  Disable bonding entirely\n\n";
    }

    virtual const char* SpecificationURL()
    {return "http://www.cec.mpg.de/forum/portal.php";} //optional

    //Flags() can return be any the following combined by | or be omitted if none apply
    // NOTREADABLE  READONEONLY  NOTWRITABLE  WRITEONEONLY
    virtual unsigned int Flags()
    {
      return READONEONLY | NOTWRITABLE;
    }

    ////////////////////////////////////////////////////
    /// The "API" interface functions
    virtual bool ReadMolecule(OBBase* pOb, OBConversion* pConv);

    string checkColumns(string tmp);
    string checkChar(string tmp, const char cTmp);
    string removeChars(string tmp, const char cTmp);
  };

  //Make an instance of the format class
  OrcaOutputFormat theOrcaOutputFormat;

  class OrcaInputFormat : public OBMoleculeFormat
  {
  public:
    //Register this format type ID
    OrcaInputFormat()
    {
      OBConversion::RegisterFormat("orcainp",this);
    }

    virtual const char* Description() //required
    {
      return
        "Orca input format\n"
        "This can be used as a template file for orca calculations\n";
    }

    virtual const char* SpecificationURL()
    {return"http://www.cec.mpg.de/forum/portal.php";} //optional

    //Flags() can return be any the following combined by | or be omitted if none apply
    // NOTREADABLE  READONEONLY  NOTWRITABLE  WRITEONEONLY
    virtual unsigned int Flags()
    {
      return NOTREADABLE | WRITEONEONLY;
    }

    ////////////////////////////////////////////////////
    /// The "API" interface functions
    virtual bool WriteMolecule(OBBase* pOb, OBConversion* pConv);

  };

  //Make an instance of the format class
  OrcaInputFormat theOrcaInputFormat;


  /////////////////////////////////////////////////////////////////
  bool OrcaOutputFormat::ReadMolecule(OBBase* pOb, OBConversion* pConv)
  {

    OBMol* pmol = pOb->CastAndClear<OBMol>();
    if(pmol==NULL)
      return false;

    //Define some references so we can use the old parameter names
    istream &ifs = *pConv->GetInStream();
    OBMol &mol = *pmol;
    const char* title = pConv->GetTitle();


    // molecule energy and gradient
    std::vector<double> FinalEnergy;
    std::vector< std::vector< vector3> > Gradients;

    //Vibrational data
    double skipFreq = 0;
    std::vector< std::vector< vector3 > > Lx;
    std::vector<double> Frequencies, Intensities, RamanActivities;
    std::vector<double> NearIRFrequencies, NearIRIntensities;
    bool NearIRDatafound = false;
    // UV data
    std::vector<double> UVWavenumber, UVWavelength, UVForces, UVEDipole;
    // CD data
    std::vector<double> CDWavelength, CDVelosity, CDStrengthsLength;
    // Absorption / Emission data or combined data
    bool OrcaSpecfound = false;
    std::vector<double> AbsWavelength, AbsCombined, AbsD2, AbsM2, AbsQ2;
    std::vector<double> EmWavelength, EmCombined, EmD2, EmM2, EmQ2;
    std::vector<double> AbsEDipole, AbsVelosity, EmEDipole, EmVelosity;
    // frequencies and normal modes
    std::vector<double> FrequenciesAll;
    int nModeAll = 0;

    //MO data
    bool m_openShell = false;
    std::vector<double>  energyEh, energyeV;
    std::vector<double>  occ;
    std::vector<double>  energyBEh, energyBeV;
    std::vector<double>  occB;

    // Conformer data
    bool newMol = false;
    double* confCoords;

    // Unit cell
    bool unitCell = false;
    std::vector<vector3> unitCellVectors;

    bool hasPartialCharges = false;
    bool geoOptRun  = false;          // flag for optimization run
    bool scanRun    = false;          // relaxed surface scan
    bool optSuccess = false;          // flag if optimazation run has converged


    char buffer[BUFF_SIZE];
    string str;
    double x,y,z;
    OBAtom *atom;

    int nAtoms = 0;

    vector<string> vs;

    int successCount =0;
    int scanCount=0;


    mol.BeginModify();
    while	(ifs.getline(buffer,BUFF_SIZE)) {
        string checkKeywords(buffer);

        if (checkKeywords.find("$$$$$$$$$$$$$$$$  JOB NUMBER") != notFound) {
            mol.EndModify();
            mol.Clear();         // new orca job - clear molecule
            mol.BeginModify();
        } // if "new orca output section"

        if (checkKeywords.find("*    Relaxed Surface Scan    *") != notFound) {
            scanRun = true;
            geoOptRun = false;
            FinalEnergy.resize(0);
            Gradients.resize(0);
            cout << "Relaxed Surface Scan = true " << endl;
            while	(ifs.getline(buffer,BUFF_SIZE)) {
                string checkNAtoms(buffer);

                if (checkNAtoms.find("Number of atoms") != notFound) {
                    tokenize(vs,buffer);
                    nAtoms = atoi((char*)vs[4].c_str());
                    break;
                }
            }
        }
        if (checkKeywords.find("Geometry Optimization Run") != notFound) {
            geoOptRun = true;
            FinalEnergy.resize(0);
            Gradients.resize(0);
            cout << "Geometry Optimization Run = true " << endl;
            while	(ifs.getline(buffer,BUFF_SIZE)) {
                string checkNAtoms(buffer);

                if (checkNAtoms.find("Number of atoms") != notFound) {
                    tokenize(vs,buffer);
                    nAtoms = atoi((char*)vs[4].c_str());
                    break;
                }
            }
        } // if "geometry optimization run"
        if (checkKeywords.find("***********************HURRAY********************") != notFound && (geoOptRun || scanRun)) {
          geoOptRun = false;    // optimization run has finished successfully
          optSuccess = true;
          successCount ++;
        } // optimization finished
        if (checkKeywords.find("RELAXED SURFACE SCAN STEP") != notFound) {
          scanRun = true;    // next scan started successfully
          optSuccess = false;
          scanCount++;
        } // next relaxed surface scan
        if (checkKeywords.find("**** RELAXED SURFACE SCAN DONE ***") != notFound) {
          scanRun = false;    // scan finished
          optSuccess = false;
        } // relaxed surface scan finished


        if (checkKeywords.find("CARTESIAN COORDINATES (ANGSTROEM)") != notFound) {
            if (unitCell) break; // dont't overwrite unit cell coordinate informations
            if (mol.NumAtoms() == 0) {
                newMol = true;
            }
            if (geoOptRun || scanRun) {
                confCoords = new double[nAtoms*3];
            }
            ifs.getline(buffer,BUFF_SIZE);	// ---- ----- ----
            ifs.getline(buffer,BUFF_SIZE);
            tokenize(vs,buffer);
            int i=0;
            while (vs.size() == 4) {

                x = atof((char*)vs[1].c_str());
                y = atof((char*)vs[2].c_str());
                z = atof((char*)vs[3].c_str());

                if (newMol){
                    atom = mol.NewAtom();
                    atom->SetAtomicNum(etab.GetAtomicNum(vs[0].c_str()));                //set atomic number
                    atom->SetVector(x,y,z); //set atom coordinates
                }
                if (geoOptRun) {
                  confCoords[i*3] = x;
                  confCoords[i*3+1] = y;
                  confCoords[i*3+2] = z;
                } else if (scanRun && optSuccess){
                    confCoords[i*3] = x;
                    confCoords[i*3+1] = y;
                    confCoords[i*3+2] = z;
                    atom = mol.GetAtom(i+1);
                    atom->SetVector(x,y,z); //set atom coordinates
                } else {
                    atom = mol.GetAtom(i+1);
                    atom->SetVector(x,y,z); //set atom coordinates
                }
                i++;
                if (!ifs.getline(buffer,BUFF_SIZE))
                    break;
                tokenize(vs,buffer);
            }
            newMol = false;
            if (geoOptRun){
              mol.AddConformer(confCoords);
            } else if (scanRun && optSuccess) {
              mol.AddConformer(confCoords);
            }
        } // if "output coordinates"

        if (checkKeywords.find("ORBITAL ENERGIES") != notFound && !geoOptRun && !scanRun) {

            energyEh.resize(0);
            energyeV.resize(0);
            occ.resize(0);
            ifs.getline(buffer,BUFF_SIZE); // skip ---------------------
            ifs.getline(buffer,BUFF_SIZE); // skip empty line or look for spin informations
            if (strstr(buffer,"SPIN UP ORBITALS") != NULL) m_openShell = true;
            ifs.getline(buffer,BUFF_SIZE); // skip headline
            ifs.getline(buffer,BUFF_SIZE);
            tokenize(vs,buffer);
            while (strstr(buffer,"---------") == NULL && vs.size() !=0) {
                if (vs.size() < 4) break;
                occ.push_back(atof(vs[1].c_str()));
                energyEh.push_back(atof(vs[2].c_str()));
                energyeV.push_back(atof(vs[3].c_str()));
                ifs.getline(buffer,BUFF_SIZE);
                tokenize(vs,buffer);
            }
            if (m_openShell) {
                energyBEh.resize(0);
                energyBeV.resize(0);
                occB.resize(0);

                ifs.getline(buffer,BUFF_SIZE); // skip spin informations
                ifs.getline(buffer,BUFF_SIZE); // skip headline
                ifs.getline(buffer,BUFF_SIZE);
                tokenize(vs,buffer);
                while (strstr(buffer,"---------") == NULL && vs.size() >0) {
                    if (vs.size() < 4) break;
                    occB.push_back(atof(vs[1].c_str()));
                    energyBEh.push_back(atof(vs[2].c_str()));
                    energyBeV.push_back(atof(vs[3].c_str()));
                    ifs.getline(buffer,BUFF_SIZE);
                    tokenize(vs,buffer);
                }
            }
        } // if "ORBITAL ENERGIES"
        if (checkKeywords.find("Total Charge") != notFound && !geoOptRun && !scanRun) {

            //get total charge

            tokenize(vs,buffer);
            if (vs.size() == 5) {
                mol.SetTotalCharge (atoi(vs[4].c_str()));
            }

            // get Multiplicity

            ifs.getline(buffer,BUFF_SIZE);
            tokenize(vs,buffer);
            if (vs.size() == 4) {
                mol.SetTotalSpinMultiplicity(atoi(vs[3].c_str()));
            }
        }
        if (checkKeywords.find("MULLIKEN ATOMIC CHARGES") != notFound && !geoOptRun && !scanRun) {
            hasPartialCharges = true;
            ifs.getline(buffer,BUFF_SIZE);	// skip --------------
            ifs.getline(buffer,BUFF_SIZE);
            str = checkChar (string(buffer), ':');  // remove ":" for correct parsing
            tokenize(vs,str);

            while (vs.size() == 3 || vs.size() == 4)
            { // atom number, atomic symbol,:,  charge, spin information (optional)

                atom = mol.GetAtom(atoi(vs[0].c_str())+1);  // Numbering starts from 0 in Orca
                atom->SetPartialCharge(atof(vs[2].c_str()));

                if (!ifs.getline(buffer,BUFF_SIZE))
                    break;
                str = checkChar (string(buffer), ':');  // remove ":" for correct parsing
                tokenize(vs,str);
            }
        } // if "MULLIKEN ATOMIC CHARGES"

        if (checkKeywords.find("FINAL SINGLE POINT ENERGY") != notFound) {
          tokenize(vs,buffer);
          if (geoOptRun) {
            if (vs.size() == 5) FinalEnergy.push_back(atof(vs[4].c_str())*_eV_kcal*_au_eV);
          } else if (scanRun && optSuccess){
            if (vs.size() == 5) {
              FinalEnergy.push_back(atof(vs[4].c_str())*_eV_kcal*_au_eV);
              mol.SetEnergy(atof(vs[4].c_str())*_eV_kcal*_au_eV);
            }
          } else {
            if (vs.size() == 5) mol.SetEnergy(atof(vs[4].c_str())*_eV_kcal*_au_eV);
          }
        } //if "FINAL SINGLE POINT ENERGY"

        if (checkKeywords.find("CARTESIAN GRADIENT") != notFound) {
          ifs.getline(buffer,BUFF_SIZE);      // skip ----------
          ifs.getline(buffer,BUFF_SIZE);      // skip empty line
          ifs.getline(buffer,BUFF_SIZE);

          tokenize(vs,buffer);
          vector<vector3> Gradient;

          while (vs.size() == 6) {

              x = atof((char*)vs[3].c_str());
              y = atof((char*)vs[4].c_str());
              z = atof((char*)vs[5].c_str());

              Gradient.push_back(vector3(x,y,z));

              if (!ifs.getline(buffer,BUFF_SIZE))
                  break;
              tokenize(vs,buffer);
          }
          if (geoOptRun) {
            Gradients.push_back(Gradient);
          } else if (scanRun && optSuccess){
              Gradients.push_back(Gradient);
          }

        } // if "CARTESIAN GRADIENTS"

        if (checkKeywords.find("VIBRATIONAL FREQUENCIES") != notFound) {
          double scaleFac = 1.;
          double tmpFreq = 0.0;
          skipFreq = 0;
            FrequenciesAll.resize(0);
            ifs.getline(buffer,BUFF_SIZE);      // skip ----------
            ifs.getline(buffer,BUFF_SIZE);      // skip empty line
            ifs.getline(buffer,BUFF_SIZE);

            tokenize(vs,buffer);

            // get scaling factor and skip next empty line (if written)
            if (vs.size() > 5) {
              scaleFac = atof(vs[5].c_str());
              ifs.getline(buffer,BUFF_SIZE);    // skip empty line
            }
            // skip zero frequencies
            for (int i=0; i<5; i++) {
              skipFreq ++;
              ifs.getline(buffer,BUFF_SIZE);  // skip first 5 lines of frequencies - always zero
            }
            ifs.getline(buffer,BUFF_SIZE);    // next frequency
            tokenize(vs,buffer);

            while (vs.size() >1) {
              tmpFreq = atof(vs[1].c_str());
              if (tmpFreq == 0.0) {
                skipFreq ++;                  // skip also this line if frequency is zero
              } else {
                FrequenciesAll.push_back(tmpFreq);
              }
              ifs.getline(buffer,BUFF_SIZE);
              tokenize(vs,buffer);
            }
            nModeAll = FrequenciesAll.size();

        } // if "VIBRATIONAL FREQUENCIES"

        if (checkKeywords.find("NORMAL MODES") != notFound) {

            Lx.resize(0);
            for (unsigned int i=0;i<6;i++) {
                ifs.getline(buffer,BUFF_SIZE);     // skip ----------, comments and blank lines
            }

            ifs.getline(buffer,BUFF_SIZE);     // header line
            tokenize(vs,buffer);
            int iMode = 0;
            while (vs.size() != 0) {
                int nColumn = vs.size();
                vector<vector<vector3> > vib;
                ifs.getline(buffer,BUFF_SIZE);
                str = checkColumns (string(buffer));
                tokenize(vs,str);
                while(vs.size() == nColumn+1) {
                    vector<double> x, y, z;
                    for (unsigned int i = 1; i < vs.size(); i++)
                        x.push_back(atof(vs[i].c_str()));
                    ifs.getline(buffer, BUFF_SIZE);
                    str = checkColumns (string(buffer));
                    tokenize(vs,str);
                    for (unsigned int i = 1; i < vs.size(); i++)
                        y.push_back(atof(vs[i].c_str()));
                    ifs.getline(buffer, BUFF_SIZE);
                    str = checkColumns (string(buffer));
                    tokenize(vs,str);
                    for (unsigned int i = 1; i < vs.size(); i++)
                        z.push_back(atof(vs[i].c_str()));

                    for (unsigned int i = 0; i < nColumn; i++) {
                        vib.push_back(vector<vector3>());
                        vib[i].push_back(vector3(x[i], y[i], z[i]));
                    }

                    ifs.getline(buffer, BUFF_SIZE);
                    str = checkColumns (string(buffer));
                    tokenize(vs,str);
                } // while

                for (unsigned int i = 0; i < nColumn; i++) {
                  if (iMode >=skipFreq) {
                    //                        std::cout <<" vib[i].size = " <<i << " " << vib[i].size() << endl;
                    Lx.push_back(vib[i]);
                    //                        std::cout << i<< "  " << Lx[i].size() << endl;
                    //                        std::cout << Lx.size() << endl;
                  }
                  iMode++;
                }
            } // while
        } // if "NORMAL MODES"}
//
// IR spectrum
//
        if (checkKeywords.find("IR SPECTRUM") != notFound) {

            Frequencies.resize(0);
            Intensities.resize(0);

            ifs.getline(buffer, BUFF_SIZE); // skip ---------------------
            ifs.getline(buffer, BUFF_SIZE); // skip empty line
            ifs.getline(buffer, BUFF_SIZE); // skip header
            ifs.getline(buffer, BUFF_SIZE);

            tokenize(vs,buffer);
            if (vs.size() > 1) {    // skip unit line if any
              ifs.getline(buffer,BUFF_SIZE);    // skip ---------------------
            }
            ifs.getline(buffer, BUFF_SIZE);
            str = checkChar (string(buffer), ':');  // remove ":" for correct parsing
            str = checkChar (str, '(');  // remove "(" for correct parsing
            tokenize(vs,str);

            while (vs.size() >= 6) {
                Frequencies.push_back(atof(vs[1].c_str()));
                Intensities.push_back(atof(vs[2].c_str()));
                ifs.getline(buffer, BUFF_SIZE);
                str = checkChar (string(buffer), ':');  // remove ":" for correct parsing
                str = checkChar (str, '(');  // remove "(" for correct parsing
                tokenize(vs,str);
            }
        } // if "IR SPECTRUM"
//
// Overtones and combined bands
//
        if (checkKeywords.find("OVERTONES AND COMBINATION BANDS") != notFound) {
            NearIRDatafound = true;
            NearIRFrequencies.resize(0);
            NearIRIntensities.resize(0);

            ifs.getline(buffer, BUFF_SIZE); // skip ---------------------
            ifs.getline(buffer, BUFF_SIZE); // skip empty line
            ifs.getline(buffer, BUFF_SIZE); // skip header
            ifs.getline(buffer, BUFF_SIZE);

            tokenize(vs,buffer);
            if (vs.size() > 1) {    // skip unit line if any
                ifs.getline(buffer,BUFF_SIZE);    // skip ---------------------
            }
            ifs.getline(buffer, BUFF_SIZE);
            str = checkChar (string(buffer), ':');  // remove ":" for correct parsing
            str = checkChar (str, '(');  // remove "(" for correct parsing
            tokenize(vs,str);

            while (vs.size() >= 6) {
                NearIRFrequencies.push_back(atof(vs[2].c_str()));
                NearIRIntensities.push_back(atof(vs[3].c_str()));
                ifs.getline(buffer, BUFF_SIZE);
                str = checkChar (string(buffer), ':');  // remove ":" for correct parsing
                str = checkChar (str, '(');  // remove "(" for correct parsing
                tokenize(vs,str);
            }
        } // if "OVERTONES AND COMBINATION BANDS"
//
// RAMAN spectrum
//
        if (checkKeywords.find("RAMAN SPECTRUM") != notFound) {

            RamanActivities.resize(0);
            ifs.getline(buffer, BUFF_SIZE); // skip ---------------------
            ifs.getline(buffer, BUFF_SIZE); // skip empty line
            ifs.getline(buffer, BUFF_SIZE); // skip header
            ifs.getline(buffer, BUFF_SIZE); // skip ---------------------
            ifs.getline(buffer, BUFF_SIZE);
            str = checkChar (string(buffer), ':');  // remove ":" for correct parsing
            tokenize(vs,str);

            while (vs.size() == 4 ) {
                RamanActivities.push_back(atof(vs[2].c_str()));
                ifs.getline(buffer, BUFF_SIZE);
                str = checkChar (string(buffer), ':');  // remove ":" for correct parsing
                tokenize(vs,str);
            }
        } // if "RAMAN SPECTRUM"
//
// ABSORPTION/EMISSION spectra
//
        if (checkKeywords.find("SPECTRUM VIA TRANSITION ELECTRIC DIPOLE MOMENTS") != notFound) {
            // Xray absorption spectrum
            if ((checkKeywords.find("X-RAY ABSORPTION") != notFound) && (checkKeywords.find("SPIN-ORBIT") == notFound)){
                OrcaSpecfound = true;
                AbsWavelength.resize(0);
                AbsEDipole.resize(0);
                ifs.getline(buffer, BUFF_SIZE); // skip ---------------------
                ifs.getline(buffer, BUFF_SIZE); // skip header
                ifs.getline(buffer, BUFF_SIZE); // skip header
                ifs.getline(buffer, BUFF_SIZE); // skip ---------------------
                ifs.getline(buffer, BUFF_SIZE);
                tokenize(vs,buffer);

                while (vs.size() == 9) {

                    AbsWavelength.push_back(1.e7/(8065.54477*atof(vs[4].c_str()))); //  convert energy in eV to wavelength in nm

                    AbsEDipole.push_back(atof(vs[5].c_str()));
                    ifs.getline(buffer, BUFF_SIZE);
                    tokenize(vs,buffer);
                }
            } //  if XRAY ABORPTION
            // XRay emision spectrum
            else if ((checkKeywords.find("X-RAY EMISSION") != notFound) && (checkKeywords.find("SPIN-ORBIT") == notFound)) {
                OrcaSpecfound = true;
                EmWavelength.resize(0);
                EmEDipole.resize(0);
                ifs.getline(buffer, BUFF_SIZE); // skip ---------------------
                ifs.getline(buffer, BUFF_SIZE); // skip header
                ifs.getline(buffer, BUFF_SIZE); // skip header
                ifs.getline(buffer, BUFF_SIZE); // skip ---------------------
                ifs.getline(buffer, BUFF_SIZE);
                tokenize(vs,buffer);

                while (vs.size() == 9) {
                    EmWavelength.push_back(1.e7/(8065.54477*atof(vs[4].c_str()))); //  convert energy in eV to wavelength in nm

                    EmEDipole.push_back(atof(vs[5].c_str()));
                    ifs.getline(buffer, BUFF_SIZE);
                    tokenize(vs,buffer);
                }
            } // if "XRAY EMISION"
            else if ((checkKeywords.find("SPIN ORBIT CORRECTED") == notFound)  && (checkKeywords.find("SOC CORRECTED") == notFound)
                     && (checkKeywords.find("TRANSIENT") == notFound)) {  // NO override with spin corrected values
              OrcaSpecfound = true;
              AbsWavelength.resize(0);
              AbsEDipole.resize(0);

                ifs.getline(buffer, BUFF_SIZE); // skip ---------------------
                ifs.getline(buffer, BUFF_SIZE); // skip header
                ifs.getline(buffer, BUFF_SIZE); // skip header
                ifs.getline(buffer, BUFF_SIZE); // skip ---------------------
                ifs.getline(buffer, BUFF_SIZE);
                tokenize(vs,buffer);

                while (vs.size() == 8) {
                    AbsWavelength.push_back(1.e7/atof(vs[1].c_str())); //  convert energy in cm-1 to wavelength in nm
                    AbsEDipole.push_back(atof(vs[3].c_str()));
                    ifs.getline(buffer, BUFF_SIZE);
                    tokenize(vs,buffer);
                }
            } // NO SPIN ORBIT CORRECTED ABSORPTION - just absorption
        } // if " SPECTRUM VIA TRANSITION ELECTRIC DIPOLE MOMENTS"

        // uv spectrum from  sTDA
        if (checkKeywords.find("excitation energies, transition moments and amplitudes") != notFound) {
            UVWavenumber.resize(0);
            UVWavelength.resize(0);
            UVForces.resize(0);
            UVEDipole.resize(0);
            ifs.getline(buffer, BUFF_SIZE); // skip blank line
            ifs.getline(buffer, BUFF_SIZE); // skip molecuar weight
            ifs.getline(buffer, BUFF_SIZE); // skip headline
            ifs.getline(buffer, BUFF_SIZE);
            tokenize(vs,buffer);

            while (vs.size() >= 7) {
                UVForces.push_back(0.0);        // ORCA doesn't have these values
                UVWavenumber.push_back(atof(vs[1].c_str()));
                UVWavelength.push_back(atof(vs[2].c_str()));
                UVEDipole.push_back(atof(vs[3].c_str()));
                ifs.getline(buffer, BUFF_SIZE);
                tokenize(vs,buffer);
            }
        } // if "excitation energies, transition moments and amplitudes"

        if (checkKeywords.find("                             ABSORPTION SPECTRUM") != notFound) {     // white spaces before ABSORPTION are necessary !!

            UVWavenumber.resize(0);
            UVWavelength.resize(0);
            UVForces.resize(0);
            UVEDipole.resize(0);
            ifs.getline(buffer, BUFF_SIZE); // skip ---------------------
            ifs.getline(buffer, BUFF_SIZE); // skip header
            ifs.getline(buffer, BUFF_SIZE); // skip header
            ifs.getline(buffer, BUFF_SIZE); // skip ---------------------
            ifs.getline(buffer, BUFF_SIZE);
            str = removeChars(string(buffer), ')');  // remove all charcters including ")" up to numbers for correct parsing
            tokenize(vs,str);

            while (vs.size() == 8) {
                UVForces.push_back(0.0);        // ORCA doesn't have these values
                UVWavenumber.push_back(atof(vs[1].c_str()));
                UVWavelength.push_back(atof(vs[2].c_str()));
                UVEDipole.push_back(atof(vs[3].c_str()));
                ifs.getline(buffer, BUFF_SIZE);
                str = removeChars(string(buffer), ')');  // remove all charcters including ")" up to numbers for correct parsing
                tokenize(vs,str);
            }
        } // if "ABSORPTION SPECTRUM"
        //
        // COMBINED ELECTRIC DIPOLE + MAGNETIC DIPOLE + ELECTRIC QUADRUPOLE SPECTRUM
        //
        if (checkKeywords.find("COMBINED ELECTRIC DIPOLE + MAGNETIC DIPOLE + ELECTRIC QUADRUPOLE") != notFound) {

            if ((checkKeywords.find("X-RAY ABSORPTION") != notFound) &&  (checkKeywords.find("(") == notFound)) {
                OrcaSpecfound = true;
                AbsWavelength.resize(0);
                AbsCombined.resize(0);
                AbsD2.resize(0);
                AbsM2.resize(0);
                AbsQ2.resize(0);
                ifs.getline(buffer, BUFF_SIZE); // skip header
                ifs.getline(buffer, BUFF_SIZE); // skip ---------------------
                ifs.getline(buffer, BUFF_SIZE); // skip header
                ifs.getline(buffer, BUFF_SIZE); // skip ---------------------
                ifs.getline(buffer, BUFF_SIZE); // skip header
                ifs.getline(buffer, BUFF_SIZE); // skip header
                ifs.getline(buffer, BUFF_SIZE); // skip ---------------------
                ifs.getline(buffer, BUFF_SIZE);
                tokenize(vs,buffer);

                while (vs.size() == 12) {
                    AbsWavelength.push_back(1.e7/(8065.54477*atof(vs[4].c_str()))); //  convert energy in eV to wavelength in nm

                    AbsCombined.push_back(atof(vs[8].c_str()));
                    AbsD2.push_back(atof(vs[9].c_str()));
                    AbsM2.push_back(atof(vs[10].c_str()));
                    AbsQ2.push_back(atof(vs[11].c_str()));
                    ifs.getline(buffer, BUFF_SIZE);
                    tokenize(vs,buffer);
                }
            } else if (checkKeywords.find("X-RAY EMISSION") != notFound) {
                OrcaSpecfound = true;
                EmWavelength.resize(0);
                EmCombined.resize(0);
                EmD2.resize(0);
                EmM2.resize(0);
                EmQ2.resize(0);
                ifs.getline(buffer, BUFF_SIZE); // skip header
                ifs.getline(buffer, BUFF_SIZE); // skip ---------------------
                ifs.getline(buffer, BUFF_SIZE); // skip header
                ifs.getline(buffer, BUFF_SIZE); // skip ---------------------
                ifs.getline(buffer, BUFF_SIZE); // skip header
                ifs.getline(buffer, BUFF_SIZE); // skip header
                ifs.getline(buffer, BUFF_SIZE); // skip ---------------------
                ifs.getline(buffer, BUFF_SIZE);
                tokenize(vs,buffer);

                while (vs.size() == 12) {
                    EmWavelength.push_back(1.e7/(8065.54477*atof(vs[4].c_str()))); //  convert energy in eV to wavelength in nm
                    EmCombined.push_back(atof(vs[8].c_str()));
                    EmD2.push_back(atof(vs[9].c_str()));
                    EmM2.push_back(atof(vs[10].c_str()));
                    EmQ2.push_back(atof(vs[11].c_str()));
                    ifs.getline(buffer, BUFF_SIZE);
                    tokenize(vs,buffer);
                }
            } else {
                OrcaSpecfound = true;

                AbsWavelength.resize(0);
                AbsCombined.resize(0);
                AbsD2.resize(0);
                AbsM2.resize(0);
                AbsQ2.resize(0);
                ifs.getline(buffer, BUFF_SIZE); // skip ---------------------
                ifs.getline(buffer, BUFF_SIZE); // skip header
                ifs.getline(buffer, BUFF_SIZE); // skip header
                ifs.getline(buffer, BUFF_SIZE); // skip ---------------------
                ifs.getline(buffer, BUFF_SIZE);
                tokenize(vs,buffer);

                while (vs.size() == 10) {
                    //                            UVForces.push_back(0.0);        // ORCA doesn't have these values
                    AbsWavelength.push_back(1.e7/atof(vs[1].c_str())); //  convert energy in cm-1 to wavelength in nm
                    AbsCombined.push_back(atof(vs[6].c_str()));
                    AbsD2.push_back(atof(vs[7].c_str()));
                    AbsM2.push_back(atof(vs[8].c_str()));
                    AbsQ2.push_back(atof(vs[9].c_str()));
                    ifs.getline(buffer, BUFF_SIZE);
                    tokenize(vs,buffer);
                }
            }
        } // if "COMBINED ELECTRIC DIPOLE + MAGNETIC DIPOLE + ELECTRIC QUADRUPOLE"
        if (checkKeywords.find("CD SPECTRUM") != notFound) {

            CDWavelength.resize(0);
            CDVelosity.resize(0);
            CDStrengthsLength.resize(0);
            ifs.getline(buffer, BUFF_SIZE); // skip ---------------------
            ifs.getline(buffer, BUFF_SIZE); // skip header
            ifs.getline(buffer, BUFF_SIZE); // skip header
            ifs.getline(buffer, BUFF_SIZE); // skip ---------------------
            ifs.getline(buffer, BUFF_SIZE);
            str = removeChars(string(buffer), ')');  // remove all charcters including ")" up to numbers for correct parsing
            tokenize(vs,str);

            while (vs.size() == 7) {
                CDVelosity.push_back(0.0);        // ORCA doesn't calculate these values
                CDWavelength.push_back(atof(vs[2].c_str()));
                CDStrengthsLength.push_back(atof(vs[3].c_str()));

                ifs.getline(buffer, BUFF_SIZE);
                str = removeChars(string(buffer), ')');  // remove all charcters including ")" up to numbers for correct parsing
                tokenize(vs,str);
            }
        } // if "CD SPECTRUM"

        if (checkKeywords.find("UNIT CELL (ANGSTROM)") != notFound) { // file contains unit cell information
            unitCellVectors.resize(0);

            ifs.getline(buffer,BUFF_SIZE);
            tokenize(vs,buffer);
            while (vs.size() == 4) {
                x = atof((char*)vs[1].c_str());
                y = atof((char*)vs[2].c_str());
                z = atof((char*)vs[3].c_str());
                unitCellVectors.push_back(vector3 (x,y,z)); //set coordinates

                if (!ifs.getline(buffer,BUFF_SIZE))
                    break;
                tokenize(vs,buffer);
            }
            if (unitCellVectors.size()!=4 )
                break;      // structure incorrect

            if (!ifs.getline(buffer,BUFF_SIZE))
                break;

            // look for coordinate information relating to the unit cell calculations

            string checkNextKeyword(buffer);
            if (checkNextKeyword.find("CARTESIAN COORDINATES (ANGSTROM)") != notFound){
                mol.Clear();

                ifs.getline(buffer,BUFF_SIZE);
                tokenize(vs,buffer);
                while (vs.size() >= 4) { // sometime there are additional infos in the line
                    atom = mol.NewAtom();
                    x = atof((char*)vs[1].c_str());
                    y = atof((char*)vs[2].c_str());
                    z = atof((char*)vs[3].c_str());
                    atom->SetVector(x,y,z); //set coordinates

                    //set atomic number
                    atom->SetAtomicNum(etab.GetAtomicNum(vs[0].c_str()));

                    if (!ifs.getline(buffer,BUFF_SIZE))
                        break;
                    tokenize(vs,buffer);
                }
            } // if "unit cell related coordinates"
            if (mol.NumAtoms() != 0)
                unitCell = true;
        } // if "unit cell information"

    } // while

    if (mol.NumAtoms() == 0) {
      mol.EndModify();
      return false;
    }

    // Attach unit cell if any

    if (unitCell) {
        OBUnitCell *uC = new OBUnitCell;

        uC->SetData(unitCellVectors.at(0), unitCellVectors.at(1), unitCellVectors.at(2));
        uC->SetOffset(unitCellVectors.at(3));
        mol.SetData(uC);
    }

    // Attach conformer data if any
    if (mol.NumConformers() > 1) {
        FinalEnergy.push_back(mol.GetEnergy()); // save also energy of scf calculation AFTER optimisation
                                                // and add them to ConformerData because endModify() will add also the lastest geometry
        OBConformerData *cd = new OBConformerData;
        cd->SetEnergies(FinalEnergy);
        if (Gradients.size() !=0) cd->SetForces(Gradients);
        mol.SetData(cd);
      //}
    }

    // Attach orbital data if any

    if (energyEh.size() > 0){
        OBOrbitalData *od = new OBOrbitalData();

        std::vector<OBOrbital> alphaOrbitals;
        int alphaHomo = 0, betaHomo = 0;
        for (unsigned int i = 0; i < energyEh.size(); i++) {
            if (occ[i]>0) alphaHomo++;
            OBOrbital orb;
            orb.SetData(energyEh[i], occ[i], " ");
            alphaOrbitals.push_back(orb);
        }
        od->SetAlphaOrbitals (alphaOrbitals);

        if (m_openShell) {
            std::vector<OBOrbital> betaOrbitals;

            for (unsigned int i = 0; i < energyBEh.size(); i++) {
                if (occ[i]>0) betaHomo++;
                OBOrbital orb;
                orb.SetData(energyBEh[i], occB[i], " ");
                betaOrbitals.push_back(orb);
            }
            od->SetBetaOrbitals (betaOrbitals);
        }
        od->SetHOMO(alphaHomo,betaHomo);
        od->SetOrigin(fileformatInput);
        mol.SetData(od);
    }

    //Attach vibrational data, if there are any, to molecule
    if(Frequencies.size()>0)
    {
        OBVibrationData* vd = new OBVibrationData;
        std::vector<double> RamanActivitiesAll, IntensitiesAll;
        if (RamanActivities.size() != 0) {
            if (nModeAll != Frequencies.size()) {
                int j=0;
                RamanActivitiesAll.resize(nModeAll,0.);
                IntensitiesAll.resize(nModeAll,0.);
                for (int i=nModeAll-Frequencies.size(); i<nModeAll; i++) {
                    IntensitiesAll.at(i) = Intensities.at(j);
                    RamanActivitiesAll.at(i) = RamanActivities.at(j);
                    j++;
                }
                vd->SetData(Lx, FrequenciesAll, IntensitiesAll, RamanActivitiesAll);
            } else {
                vd->SetData(Lx, Frequencies, Intensities, RamanActivities);
            }
        } else {
            if (nModeAll != Frequencies.size()) {
                int j=0;
                IntensitiesAll.resize(nModeAll,0.);
                for (int i=nModeAll-Frequencies.size(); i<nModeAll; i++) {
                    IntensitiesAll.at(i) = Intensities.at(j);
                    j++;
                }
                vd->SetData(Lx, FrequenciesAll, IntensitiesAll);
            } else {
                vd->SetData(Lx, Frequencies, Intensities);
            }
        }
        mol.SetData(vd);
    }

    // Attach UV / CD spectra data if there are any

    if (OrcaSpecfound || (AbsCombined.size() !=0)) {
        OBOrcaSpecData* orcaSpec = new OBOrcaSpecData;
        orcaSpec->SetSpecData(OrcaSpecfound);
        if (AbsWavelength.size() != 0)  {
            orcaSpec->SetAbsWavelength(AbsWavelength);
            if (OrcaSpecfound) {
                if (AbsEDipole.size() != 0)   orcaSpec->SetAbsEDipole(AbsEDipole);
                if (AbsVelosity.size() != 0)   orcaSpec->SetAbsVelocity(AbsVelosity);
            }
            if (AbsCombined.size() != 0) {
                orcaSpec->SetAbsCombined(AbsCombined);
                orcaSpec->SetAbsD2(AbsD2);
                orcaSpec->SetAbsM2(AbsM2);
                orcaSpec->SetAbsQ2(AbsQ2);
            }
        }
        if (OrcaSpecfound && (EmWavelength.size() != 0))  {
            orcaSpec->SetEmWavelength(EmWavelength);
            if (EmEDipole.size() != 0)   orcaSpec->SetEmEDipole(EmEDipole);
            if (EmVelosity.size() != 0)   orcaSpec->SetEmVelosity(EmVelosity);
            if (EmCombined.size() != 0) {
                orcaSpec->SetEmCombined(EmCombined);
                orcaSpec->SetEmD2(EmD2);
                orcaSpec->SetEmM2(EmM2);
                orcaSpec->SetEmQ2(EmQ2);
            }
        }

        orcaSpec->SetOrigin(fileformatInput);
        mol.SetData(orcaSpec);
    }
    if (NearIRDatafound) {
        OBOrcaNearIRData* nearIRData = new OBOrcaNearIRData;
        nearIRData->SetFrequencies(NearIRFrequencies);
        nearIRData->SetIntensities(NearIRIntensities);
        nearIRData->SetNearIRData(NearIRDatafound);
        nearIRData->SetOrigin(fileformatInput);
        mol.SetData(nearIRData);
    }
    if(UVWavelength.size() > 0 || CDWavelength.size() > 0)
    {
        OBElectronicTransitionData* etd = new OBElectronicTransitionData;

        if (UVWavelength.size() > 0) {
            // UV spectrum has been found
            etd->SetData(UVWavelength, UVForces);
            if (UVEDipole.size() == UVWavelength.size())
                etd->SetEDipole(UVEDipole);
            // additional CD spectrum has also been found
            if (CDWavelength.size() == UVWavelength.size()) {
                etd->SetRotatoryStrengthsLength(CDStrengthsLength);
                etd->SetRotatoryStrengthsVelocity(CDVelosity); // just vector with 0.0 because ORCA doesn't calculate these values
            }
        } else {
            // only CD spectrum has been found
            etd->SetData(CDWavelength, CDVelosity); // only wavelengths information are known , 2nd vector just contains 0.0
            etd->SetRotatoryStrengthsLength(CDStrengthsLength);
            etd->SetRotatoryStrengthsVelocity(CDVelosity); // just vector with 0.0 because ORCA doesn't calculate these values
        }
        etd->SetOrigin(fileformatInput);
        mol.SetData(etd);
    }

    if (!pConv->IsOption("b",OBConversion::INOPTIONS))
      mol.ConnectTheDots();

    if (!pConv->IsOption("s",OBConversion::INOPTIONS) && !pConv->IsOption("b",OBConversion::INOPTIONS))
      mol.PerceiveBondOrders();

    mol.EndModify();

    if (hasPartialCharges)
      mol.SetPartialChargesPerceived();
    mol.SetTitle(title);

    return(true);
  }

  ////////////////////////////////////////////////////////////////

  bool OrcaInputFormat::WriteMolecule(OBBase* pOb, OBConversion* pConv)
  {
    OBMol* pmol = dynamic_cast<OBMol*>(pOb);
    if(pmol==NULL)
      return false;

    //Define some references so we can use the old parameter names
    ostream &ofs = *pConv->GetOutStream();
    OBMol &mol = *pmol;

    char buffer[BUFF_SIZE];

    ofs << "# ORCA input file" << endl;
    ofs << "# " << mol.GetTitle() << endl;
    ofs << "! insert inline commands here " << endl;
    ofs << "* xyz " << mol.GetTotalCharge() << " " << mol.GetTotalSpinMultiplicity() << endl;


    FOR_ATOMS_OF_MOL(atom, mol)
    {
        ofs << setw(4) << right
            << OpenBabel::etab.GetSymbol(atom->GetAtomicNum())
            << setw(15) << setprecision(5) << fixed << showpoint
            << right << atom->GetX() << " " << setw(15) << atom->GetY() << " "
            << setw(15) << atom->GetZ() << endl;
    }

    ofs << "*" << endl;

    return(true);
  }

// small function to avoid wrong parsing
// if there is no whitespace between the numbers in the column structure
#ifdef _MSC_VER
  string OrcaOutputFormat::checkColumns(string checkBuffer)
  {
    string pattern ("[0-9]-");
    std::tr1::regex myregex;
    std::smatch pm;
    try {
      myregex.assign(pattern,
                     std::tr1::regex_constants::extended);
      //iok = true;
    } catch (std::tr1::regex_error ex) {
        return (checkBuffer); // do nothing
      //iok = false;
    }
    while (std::regex_search (checkBuffer,pm,myregex)) {
        checkBuffer.insert(pm.position(0)+1, " ");
    }
    return (checkBuffer);
  }
#else
  string OrcaOutputFormat::checkColumns(string checkBuffer)
  {
      string pattern ("[0-9]-");
      regmatch_t pm;
      regex_t myregex;
      int pos = regcomp(&myregex, pattern.c_str(), REG_EXTENDED);
      if (pos !=0) return (checkBuffer); // do nothing

      while (regexec(&myregex, checkBuffer.c_str(), 1, &pm, REG_EXTENDED) == 0) {
          checkBuffer.insert(pm.rm_eo-1, " ");  // insert whitespace to seperate the columns
      }
      return (checkBuffer);
  }
#endif
//
// remove special characters from input string
//
  string OrcaOutputFormat::checkChar(string checkBuffer, const char specChar)
  {
      size_t pos;
//      cout << "checkBuffer = " << checkBuffer << endl;
//      cout << "specChar = " << specChar << endl;
//      cout << "(checkBuffer.find(specChar) = " << checkBuffer.find(specChar) << endl;
      pos = checkBuffer.find(specChar);
      while (pos != notFound) {
          checkBuffer.replace(pos, 1, " ");
          pos = checkBuffer.find(specChar);
//          cout << "checkBuffer = " << checkBuffer << endl;
      }

      return (checkBuffer);
  }
  //
  // remove all characters up to special character from input string
  //
  string OrcaOutputFormat::removeChars(string checkBuffer, const char specChar)
  {
      size_t pos;
      string corrString;
//      cout << "checkBuffer = " << checkBuffer << endl;
//      cout << "specChar = " << specChar << endl;
//      cout << "(checkBuffer.find(specChar) = " << checkBuffer.find_last_of(specChar) << endl;
      pos = checkBuffer.find_last_of(specChar);
      corrString = checkBuffer.substr(pos+1);
//      cout << "corrString = " << corrString << endl;

      return (corrString);
  }
} //namespace OpenBabel
