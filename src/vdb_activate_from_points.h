#ifndef __SOP_vdb_activate_from_points_h__
#define __SOP_vdb_activate_from_points_h__

#include <SOP/SOP_Node.h>

namespace VdbActivateFromPoints {
class SOP_VdbActivateFromPoints : public SOP_Node
{
public:
    // node contructor for HDK
    static OP_Node *myConstructor(OP_Network*, const char *, OP_Operator *);

    // parameter array for Houdini UI
    static PRM_Template myTemplateList[];

protected:
    // constructor, destructor
    SOP_VdbActivateFromPoints(OP_Network *net, const char *name, OP_Operator *op);

    virtual ~SOP_VdbActivateFromPoints();

    // labeling node inputs in Houdini UI
    virtual const char *inputLabel(unsigned idx) const;

    // main function that does geometry processing
    virtual OP_ERROR cookMySop(OP_Context &context);

private:
    // helper function for returning value of parameter
    int DEBUG() { return evalInt("debug", 0, 0); }

};
}

#endif