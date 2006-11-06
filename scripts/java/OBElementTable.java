/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.30
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */


public class OBElementTable extends OBGlobalDataBase {
  private long swigCPtr;

  protected OBElementTable(long cPtr, boolean cMemoryOwn) {
    super(net.sourceforge.openbabelJNI.SWIGOBElementTableUpcast(cPtr), cMemoryOwn);
    swigCPtr = cPtr;
  }

  protected static long getCPtr(OBElementTable obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if(swigCPtr != 0 && swigCMemOwn) {
      swigCMemOwn = false;
      net.sourceforge.openbabelJNI.delete_OBElementTable(swigCPtr);
    }
    swigCPtr = 0;
    super.delete();
  }

  public OBElementTable() {
    this(net.sourceforge.openbabelJNI.new_OBElementTable(), true);
  }

  public void ParseLine(String arg0) {
    net.sourceforge.openbabelJNI.OBElementTable_ParseLine(swigCPtr, this, arg0);
  }

  public long GetNumberOfElements() {
    return net.sourceforge.openbabelJNI.OBElementTable_GetNumberOfElements(swigCPtr, this);
  }

  public long GetSize() {
    return net.sourceforge.openbabelJNI.OBElementTable_GetSize(swigCPtr, this);
  }

  public int GetAtomicNum(String arg0) {
    return net.sourceforge.openbabelJNI.OBElementTable_GetAtomicNum__SWIG_0(swigCPtr, this, arg0);
  }

  public int GetAtomicNum(String arg0, SWIGTYPE_p_int iso) {
    return net.sourceforge.openbabelJNI.OBElementTable_GetAtomicNum__SWIG_1(swigCPtr, this, arg0, SWIGTYPE_p_int.getCPtr(iso));
  }

  public String GetSymbol(int arg0) {
    return net.sourceforge.openbabelJNI.OBElementTable_GetSymbol(swigCPtr, this, arg0);
  }

  public double GetVdwRad(int arg0) {
    return net.sourceforge.openbabelJNI.OBElementTable_GetVdwRad(swigCPtr, this, arg0);
  }

  public double GetCovalentRad(int arg0) {
    return net.sourceforge.openbabelJNI.OBElementTable_GetCovalentRad(swigCPtr, this, arg0);
  }

  public double GetMass(int arg0) {
    return net.sourceforge.openbabelJNI.OBElementTable_GetMass(swigCPtr, this, arg0);
  }

  public double CorrectedBondRad(int arg0, int arg1) {
    return net.sourceforge.openbabelJNI.OBElementTable_CorrectedBondRad__SWIG_0(swigCPtr, this, arg0, arg1);
  }

  public double CorrectedBondRad(int arg0) {
    return net.sourceforge.openbabelJNI.OBElementTable_CorrectedBondRad__SWIG_1(swigCPtr, this, arg0);
  }

  public double CorrectedVdwRad(int arg0, int arg1) {
    return net.sourceforge.openbabelJNI.OBElementTable_CorrectedVdwRad__SWIG_0(swigCPtr, this, arg0, arg1);
  }

  public double CorrectedVdwRad(int arg0) {
    return net.sourceforge.openbabelJNI.OBElementTable_CorrectedVdwRad__SWIG_1(swigCPtr, this, arg0);
  }

  public int GetMaxBonds(int arg0) {
    return net.sourceforge.openbabelJNI.OBElementTable_GetMaxBonds(swigCPtr, this, arg0);
  }

  public double GetElectroNeg(int arg0) {
    return net.sourceforge.openbabelJNI.OBElementTable_GetElectroNeg(swigCPtr, this, arg0);
  }

  public double GetIonization(int arg0) {
    return net.sourceforge.openbabelJNI.OBElementTable_GetIonization(swigCPtr, this, arg0);
  }

  public double GetElectronAffinity(int arg0) {
    return net.sourceforge.openbabelJNI.OBElementTable_GetElectronAffinity(swigCPtr, this, arg0);
  }

  public vectorDouble GetRGB(int arg0) {
    return new vectorDouble(net.sourceforge.openbabelJNI.OBElementTable_GetRGB(swigCPtr, this, arg0), true);
  }

  public String GetName(int arg0) {
    return net.sourceforge.openbabelJNI.OBElementTable_GetName(swigCPtr, this, arg0);
  }

}
