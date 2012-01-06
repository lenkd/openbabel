/**********************************************************************
Copyright (C) 2011 by Albert DeFusco

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
#include <openbabel/obiter.h>

#include <sstream>
#include <unordered_map>

using namespace std;
namespace OpenBabel
{

  class LMPDATFormat : public OBMoleculeFormat
  {
  public:
    //Register this format type ID
    LMPDATFormat()
    {
      OBConversion::RegisterFormat("lmpdat", this, "chemical/x-lmpdat");
      OBConversion::RegisterOptionParam("q", NULL, 1, OBConversion::OUTOPTIONS);
      OBConversion::RegisterOptionParam("d", NULL, 1, OBConversion::OUTOPTIONS);
    }

    virtual const char* Description() //required
    {
      return
	"The LAMMPS data format\n"
        "Write Options, e.g. -xq\n"
	"  q \"water-model\" Set atomic charges for water:\n"
	"      SPC (default), SPCE\n"
	"  d \"length\" Set the lenght of the boundary box\n"
	"    around the molecule.\n"
	"    The default is to make a cube around the molecule\n"
	"    adding 50\% to the most positive and negative\n"
	"    cartesian coordinate.\n"
	;
    };


    virtual const char* GetMIMEType()
    { return "chemical/x-lmpdat"; };

    virtual unsigned int Flags()
    {
      return NOTREADABLE | WRITEONEONLY;
    };

    virtual bool WriteMolecule(OBBase* pOb, OBConversion* pConv);
  };
  //***

  //Make an instance of the format class
  LMPDATFormat theLMPDATFormat;

  ////////////////////////////////////////////////////////////////

  bool LMPDATFormat::WriteMolecule(OBBase* pOb, OBConversion* pConv)
  {
    OBMol* pmol = dynamic_cast<OBMol*>(pOb);
    if(pmol==NULL)
      return false;

    //Define some references so we can use the old parameter names
    ostream &ofs = *pConv->GetOutStream();
    OBMol &mol = *pmol;
    OBAtom *a;
    OBAtom *b;
    OBAtom *c;
    OBAtom *d;

    char buffer[BUFF_SIZE];


    //Very basic atom typing by element only
    //TODO: The maps should become smarts strings instead of element names
    OBAtom *atom;
    double ThisMass;
    string ThisAtom;
    unordered_map<string, double> AtomMass;
    FOR_ATOMS_OF_MOL(atom, mol)
    {
		 ThisMass=atom->GetAtomicMass();
		 ThisAtom=etab.GetSymbol(atom->GetAtomicNum());
		 AtomMass[ThisAtom] = ThisMass;
    }
    unordered_map<string, int> AtomType;
    int AtomIndex=0;
    unordered_map<string, double>::iterator itr;
    //Set AtomType integer
    for(itr=AtomMass.begin();itr!=AtomMass.end();++itr) 
    {
	    AtomIndex++;
	    AtomType[itr->first] = AtomIndex;
    }

    //Determine unique bonds
    mol.ConnectTheDots();
    OBBond *bond;
    char BondString[BUFF_SIZE];
    unordered_map<string, int> BondType;
    FOR_BONDS_OF_MOL(bond,mol)
    {
	    a=bond->GetBeginAtom();
	    b=bond->GetEndAtom();

	    //To let the unordered map determine unique keys,
	    //always put the heavier atom first
	    if(b->GetAtomicNum() > a->GetAtomicNum() )
	    {
		    snprintf(BondString,BUFF_SIZE,
				    "%2s:%2s",
				    etab.GetSymbol(b->GetAtomicNum()),
				    etab.GetSymbol(a->GetAtomicNum()));
	    }
	    else
	    {
		    snprintf(BondString,BUFF_SIZE,
				    "%2s:%2s",
				    etab.GetSymbol(a->GetAtomicNum()),
				    etab.GetSymbol(b->GetAtomicNum()));
	    }
	    BondType[BondString] = 0;
    }
    unordered_map<string, int>::iterator intitr;
    int BondIndex=0;
    //Set the BondType integer
    for(intitr=BondType.begin();intitr!=BondType.end();++intitr)
    {
	    BondIndex++;
	    BondType[intitr->first] = BondIndex;
    }

    //Determine unique angles
    mol.FindAngles();
    OBAngle *angle;
    int anglecount=0;
    char AngleString[BUFF_SIZE];
    unordered_map<string, int> AngleType;
    FOR_ANGLES_OF_MOL(angle,mol)
    {
	    anglecount++;
	    a = mol.GetAtom((*angle)[0]+1);
	    b = mol.GetAtom((*angle)[1]+1);
	    c = mol.GetAtom((*angle)[2]+1);
	    //Origanization trick:
	    //1. The atom "a" is acutally the middle
	    //2. Always write the heavy element first
	    if(b->GetAtomicNum() > c->GetAtomicNum() ) 
	    {
		    snprintf(AngleString,BUFF_SIZE,
				    "%2s:%2s:%2s",
				    etab.GetSymbol(b->GetAtomicNum()),
				    etab.GetSymbol(a->GetAtomicNum()),
				    etab.GetSymbol(c->GetAtomicNum()));
	    }
	    else
	    {
		    snprintf(AngleString,BUFF_SIZE,
				    "%2s:%2s:%2s",
				    etab.GetSymbol(c->GetAtomicNum()),
				    etab.GetSymbol(a->GetAtomicNum()),
				    etab.GetSymbol(b->GetAtomicNum()));
	    }
	    AngleType[AngleString]=0;
    }
    int AngleIndex=0;
    //Set the AtomType integer
    for(intitr=AngleType.begin();intitr!=AngleType.end();++intitr)
    {
	    AngleIndex++;
	    AngleType[intitr->first] = AngleIndex;
    }

    //dihedrals
    mol.FindTorsions();
    OBTorsion *dihedral;
    int dihedralcount=0;
    char DihedralString[BUFF_SIZE];
    unordered_map<string, int>DihedralType;
    FOR_TORSIONS_OF_MOL(dihedral, mol)
    {
	    dihedralcount++;
	    a = mol.GetAtom((*dihedral)[0]+1);
	    b = mol.GetAtom((*dihedral)[1]+1);
	    c = mol.GetAtom((*dihedral)[2]+1);
	    d = mol.GetAtom((*dihedral)[3]+1);
	    //place the heavier element first
	    //the same may have to be done with the inner two as well
	    if(a->GetAtomicNum() > d->GetAtomicNum() )
	    {
		    snprintf(DihedralString,BUFF_SIZE,
				    "%2s:%2s:%2s:%2s",
				    etab.GetSymbol(a->GetAtomicNum()),
				    etab.GetSymbol(b->GetAtomicNum()),
				    etab.GetSymbol(c->GetAtomicNum()),
				    etab.GetSymbol(d->GetAtomicNum()));
	    }
	    else
	    {
		    snprintf(DihedralString,BUFF_SIZE,
				    "%2s:%2s:%2s:%2s",
				    etab.GetSymbol(d->GetAtomicNum()),
				    etab.GetSymbol(b->GetAtomicNum()),
				    etab.GetSymbol(c->GetAtomicNum()),
				    etab.GetSymbol(a->GetAtomicNum()));
	    }
	    DihedralType[DihedralString]=0;
    }
    int DihedralIndex=0;
    //Set DihedralType integer
    for(intitr=DihedralType.begin();intitr!=DihedralType.end();++intitr)
    {
	    DihedralIndex++;
	    DihedralType[intitr->first] = DihedralIndex;
    }

    //The box lengths
    vector3 vmin,vmax;
    vmax.Set(-10E10,-10E10,-10E10);
    vmin.Set( 10E10, 10E10, 10E10);
    FOR_ATOMS_OF_MOL(a,mol)
    {
        if (a->x() < vmin.x())
            vmin.SetX(a->x());
        if (a->y() < vmin.y())
            vmin.SetY(a->y());
        if (a->z() < vmin.z())
            vmin.SetZ(a->z());

        if (a->x() > vmax.x())
            vmax.SetX(a->x());
        if (a->y() > vmax.y())
            vmax.SetY(a->y());
        if (a->z() > vmax.z())
            vmax.SetZ(a->z());
    }

    //For now, a simple cube may be the best way to go.
    //It may be necessary to set the boxlenght to enforce
    //a density.
    const char *boxLn = pConv->IsOption("d",OBConversion::OUTOPTIONS);
    double xlo,xhi;
    char BoxString[BUFF_SIZE];
    if(boxLn)
    {
	    xlo=-atof(boxLn);
	    xhi=atof(boxLn);
    }
    else
    {
	    double cmin=10e-10;
	    double cmax=-10e10;
	    if ( vmax.x() > cmax ) cmax=vmax.x();
	    if ( vmax.y() > cmax ) cmax=vmax.y();
	    if ( vmax.z() > cmax ) cmax=vmax.z();
	    if ( vmin.x() < cmin ) cmin=vmin.x();
	    if ( vmin.y() < cmin ) cmin=vmin.y();
	    if ( vmin.z() < cmin ) cmin=vmin.z();

	    double length=cmax-cmin;
	    xlo = cmin -0.5;//- 0.01*length;
	    xhi = cmax +0.5;//+ 0.01*length;
    }
    snprintf(BoxString,BUFF_SIZE,
		    "%10.5f %10.5f xlo xhi\n%10.5f %10.5f ylo yhi\n%10.5f %10.5f zlo zhi\n",
		    xlo,xhi,
		    xlo,xhi,
		    xlo,xhi);
    

    //%%%%%%%%%%%%%% Now writting the data file %%%%%%%%%%%%%

    //The LAMMPS header stars here
    ofs << "LAMMPS data file generated by OpenBabel" << endl;
    ofs << mol.NumAtoms() << " atoms"     << endl;
    ofs << mol.NumBonds() << " bonds"     << endl;
    ofs << anglecount     << " angles"    << endl; // no NumAngles()?
    ofs << dihedralcount  << " dihedrals" << endl;
    ofs << 0              << " impropers" << endl;
    ofs << AtomType.size()     << " atom types"     << endl;
    ofs << BondType.size()     << " bond types"     << endl;
    ofs << AngleType.size()    << " angle types"    << endl;
    ofs << DihedralType.size() << " dihedral types" << endl;
    ofs << 0                   << " improper types" << endl;
    ofs << BoxString << endl;


    //Write the atom types
    ofs << endl << endl << "Masses" << endl << endl;
    for(itr=AtomMass.begin();itr!=AtomMass.end();++itr) 
    {
	    double mass=itr->second;
	    ofs << AtomType[itr->first] << " " << mass << " # " << itr->first << endl;
    }
    ofs << endl;



    //Set atomic charges for atom_style=full
    //These are charges for the SPC water model
    const char *selectCharges = pConv->IsOption("q",OBConversion::OUTOPTIONS);
    unordered_map<string, double> AtomCharge;
    for(itr=AtomMass.begin();itr!=AtomMass.end();++itr) 
    {
	    if(selectCharges)
	    {
		    if(strcmp(selectCharges,"spce")==0)
		    {
			    if(itr->second == 15.9994)
				    AtomCharge[itr->first] = -0.8472;
			    if(itr->second == 1.00794) 
				    AtomCharge[itr->first] =  0.4236;
		    }
		    else if(strcmp(selectCharges,"spc")==0)
		    {
			    if(itr->second == 15.9994)
				    AtomCharge[itr->first] = -0.820;
			    if(itr->second == 1.00794) 
				    AtomCharge[itr->first] =  0.410;
		    }
	    }
	    else
	    {
		    if(itr->second == 15.9994)
			    AtomCharge[itr->first] = -0.820;
		    if(itr->second == 1.00794) 
			    AtomCharge[itr->first] =  0.410;
	    }

    }



    //Write atomic coordinates
    vector<OBMol>           mols;
    vector<OBMol>::iterator molitr;
    mols=mol.Separate();
    int atomcount=0;
    int molcount=0;
    ofs << endl;
    ofs << "Atoms" << endl << endl;
    snprintf(buffer,BUFF_SIZE,"#%3s %4s %4s %10s %10s %10s %10s\n",
		    "idx","mol","type","charge","x","y","z");
    //ofs << buffer;
    for(molitr=mols.begin();molitr!=mols.end();++molitr)
    {
	    molcount++;
	    FOR_ATOMS_OF_MOL(atom,*molitr) 
	    {
		    int atomid=5;
		    double charge=0.5;
		    atomcount++;
		    ThisAtom=etab.GetSymbol(atom->GetAtomicNum());
		    snprintf(buffer,BUFF_SIZE,"%-4d %4d %4d %10.5f %10.5f %10.5f %10.5f # %3s\n",
				    atomcount,molcount,
				    AtomType[ThisAtom],
				    AtomCharge[ThisAtom],
				    atom->GetX(),
				    atom->GetY(),
				    atom->GetZ(),
				    ThisAtom.c_str());
		    ofs << buffer;
	    }
    } 


    //Write Bonds
    BondIndex=0;
    ofs << endl << endl;
    ofs << "Bonds" << endl;
    ofs << endl;
    snprintf(buffer,BUFF_SIZE,
		    "#%3s %4s %4s %4s\n",
		    "idx","type","atm1","atom2");
    //ofs << buffer;
    FOR_BONDS_OF_MOL(bond,mol)
    {
	    BondIndex++;
	    a = bond->GetBeginAtom();
	    b = bond->GetEndAtom();
	    //To let the unordered map determine unique keys,
	    //always put the heavier atom first
	    if(b->GetAtomicNum() > a->GetAtomicNum() )
	    {
		    snprintf(BondString,BUFF_SIZE,
				    "%2s:%2s",
				    etab.GetSymbol(b->GetAtomicNum()),
				    etab.GetSymbol(a->GetAtomicNum()));
		    snprintf(buffer,BUFF_SIZE,
				    "%-4d %4d %4d %4d # %5s\n",
				    BondIndex,
				    BondType[BondString],
				    bond->GetEndAtomIdx(),
				    bond->GetBeginAtomIdx(),
				    BondString);
	    }
	    else
	    {
		    snprintf(BondString,BUFF_SIZE,
				    "%2s:%2s",
				    etab.GetSymbol(a->GetAtomicNum()),
				    etab.GetSymbol(b->GetAtomicNum()));
		    snprintf(buffer,BUFF_SIZE,
				    "%-4d %4d %4d %4d # %5s\n",
				    BondIndex,
				    BondType[BondString],
				    bond->GetBeginAtomIdx(),
				    bond->GetEndAtomIdx(),
				    BondString);
	    }
	    ofs << buffer;
    }
    ofs << endl;

    //Write Angles
    AngleIndex=0;
    ofs << endl;
    ofs << "Angles" << endl;
    ofs << endl;
    snprintf(buffer,BUFF_SIZE,
		    "#%3s %4s %4s %4s %4s\n",
		    "idx","type","atm1","atm2","atm3");
    //ofs << buffer;
    FOR_ANGLES_OF_MOL(angle,mol)
    {
	    AngleIndex++;
	    a = mol.GetAtom((*angle)[0]+1);
	    b = mol.GetAtom((*angle)[1]+1);
	    c = mol.GetAtom((*angle)[2]+1);
	    //Origanization trick:
	    //1. The atom "a" is acutally the middle
	    //2. Always write the heavy element first
	    if(b->GetAtomicNum() > c->GetAtomicNum() ) 
	    {
		    snprintf(AngleString,BUFF_SIZE,
				    "%2s:%2s:%2s",
				    etab.GetSymbol(b->GetAtomicNum()),
				    etab.GetSymbol(a->GetAtomicNum()),
				    etab.GetSymbol(c->GetAtomicNum()));
		    snprintf(buffer,BUFF_SIZE,
				    "%-4d %4d %4d %4d %4d # %8s\n",
				    AngleIndex,
				    AngleType[AngleString],
				    b->GetIdx(),
				    a->GetIdx(),
				    c->GetIdx(),
				    AngleString);
	    }
	    else
	    {
		    snprintf(AngleString,BUFF_SIZE,
				    "%2s:%2s:%2s",
				    etab.GetSymbol(c->GetAtomicNum()),
				    etab.GetSymbol(a->GetAtomicNum()),
				    etab.GetSymbol(b->GetAtomicNum()));
		    snprintf(buffer,BUFF_SIZE,
				    "%-4d %4d %4d %4d %4d # %8s\n",
				    AngleIndex,
				    AngleType[AngleString],
				    c->GetIdx(),
				    a->GetIdx(),
				    b->GetIdx(),
				    AngleString);
	    }
	    ofs << buffer;

    }
    ofs << endl;

    //Write Dihedrals
    if(dihedralcount>0)
    {
	    ofs << endl;
	    ofs << "Dihedrals" << endl;
	    ofs << endl;
	    snprintf(buffer,BUFF_SIZE,
			    "#%3s %4s %4s %4s %4s %4s\n",
			    "idx","type","atm1","atm2","atm3","atm4");
	    //ofs << buffer;
	    DihedralIndex=0;
	    FOR_TORSIONS_OF_MOL(dihedral, mol)
	    {
		    DihedralIndex++;
		    a = mol.GetAtom((*dihedral)[0]+1);
		    b = mol.GetAtom((*dihedral)[1]+1);
		    c = mol.GetAtom((*dihedral)[2]+1);
		    d = mol.GetAtom((*dihedral)[3]+1);
		    //place the heavier element first
		    //the same may have to be done with the inner two as well
		    if(a->GetAtomicNum() > d->GetAtomicNum() )
		    {
			    snprintf(DihedralString,BUFF_SIZE,
					    "%2s:%2s:%2s:%2s",
					    etab.GetSymbol(a->GetAtomicNum()),
					    etab.GetSymbol(b->GetAtomicNum()),
					    etab.GetSymbol(c->GetAtomicNum()),
					    etab.GetSymbol(d->GetAtomicNum()));
			    snprintf(buffer,BUFF_SIZE,
					    "%-4d %4d %4d %4d %4d %4d # %11s\n",
					    DihedralIndex,
					    DihedralType[DihedralString],
					    a->GetIdx(),
					    b->GetIdx(),
					    c->GetIdx(),
					    d->GetIdx(),
					    DihedralString);
		    }
		    else
		    {
			    snprintf(DihedralString,BUFF_SIZE,
					    "%2s:%2s:%2s:%2s",
					    etab.GetSymbol(d->GetAtomicNum()),
					    etab.GetSymbol(b->GetAtomicNum()),
					    etab.GetSymbol(c->GetAtomicNum()),
					    etab.GetSymbol(a->GetAtomicNum()));
			    snprintf(buffer,BUFF_SIZE,
					    "%-4d %4d %4d %4d %4d %4d # %11s\n",
					    DihedralIndex,
					    DihedralType[DihedralString],
					    d->GetIdx(),
					    b->GetIdx(),
					    c->GetIdx(),
					    a->GetIdx(),
					    DihedralString);
		    }
		    ofs << buffer;
	    }
    }


    return(true);
  }

} //namespace OpenBabel
