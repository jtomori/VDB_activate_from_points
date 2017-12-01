#include "vdb_activate_from_points.h"

#include <limits.h>
#include <SYS/SYS_Math.h>

#include <UT/UT_DSOVersion.h>
#include <UT/UT_Interrupt.h>

#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>

#include <GU/GU_Detail.h>
#include <GEO/GEO_PrimPoly.h>

#include <PRM/PRM_Include.h>
#include <CH/CH_LocalVariable.h>

#include <OP/OP_AutoLockInputs.h>

#include <GU/GU_PrimVDB.h>

#include <openvdb/openvdb.h>


using namespace VdbActivateFromPoints;

// register the operator in Houdini, it is a hook ref for Houdini
void newSopOperator(OP_OperatorTable *table)
{
    OP_Operator *op;

	op = new OP_Operator(
    		"vdbActivateFromPoints",                      // internal name, needs to be unique in OP_OperatorTable (table containing all nodes for a network type - SOPs in our case, each entry in the table is an object of class OP_Operator which basically defines everything Houdini requires in order to create nodes of the new type)
    		"VDB Activate from Points",                   // UI name
    		SOP_VdbActivateFromPoints::myConstructor,     // how to build the node - A class factory function which constructs nodes of this type
    		SOP_VdbActivateFromPoints::myTemplateList,    // my parameters - An array of PRM_Template objects defining the parameters to this operator
    		2,                                            // min # of sources
    		2);                                           // max # of sources

    // place this operator under the VDB submenu in the TAB menu.
    op->setOpTabSubMenuPath("VDB");

    // after addOperator(), 'table' will take ownership of 'op'
    table->addOperator(op);
}

// label node inputs, 0 corresponds to first input, 1 to the second one
const char *
SOP_VdbActivateFromPoints::inputLabel(unsigned idx) const
{
    switch (idx){
        case 0: return "VDB";
        case 1: return "Points where active voxels should be";
        default: return "default";
    }
}

// define parameter for debug option
static PRM_Name debugPRM("debug", "Print debug information"); // internal name, UI name

// assign parameter to the interface, which is array of PRM_Template objects
PRM_Template SOP_VdbActivateFromPoints::myTemplateList[] = 
{
    PRM_Template(PRM_TOGGLE, 1, &debugPRM, PRMzeroDefaults), // type (checkbox), size (one in our case, but rgb/xyz values would need 3), pointer to a PRM_Name describing the parameter name, default value (0 - disabled)
    PRM_Template() // at the end there needs to be one empty PRM_Template object
};

// constructors, destructors, usually there is no need to really modify anything here, the constructor's job is to ensure the node is put into the proper network
OP_Node * 
SOP_VdbActivateFromPoints::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new SOP_VdbActivateFromPoints(net, name, op);
}

SOP_VdbActivateFromPoints::SOP_VdbActivateFromPoints(OP_Network *net, const char *name, OP_Operator *op) : SOP_Node(net, name, op) {}

SOP_VdbActivateFromPoints::~SOP_VdbActivateFromPoints() {}

// function that does the actual job
OP_ERROR
SOP_VdbActivateFromPoints::cookMySop(OP_Context &context)
{
    // we must lock our inputs before we try to access their geometry, OP_AutoLockInputs will automatically unlock our inputs when we return
    OP_AutoLockInputs inputs(this);
    if (inputs.lock(context) >= UT_ERROR_ABORT)
        return error();

    // duplicate our incoming geometry
    duplicateSource(0, context);

    // check for interrupt - interrupt scope closes automatically when 'progress' is destructed.
    UT_AutoInterrupt progress("Activating voxels...");

    // get pointer to geometry from second input
    const GU_Detail *points = inputGeo(1);

    // check if debug parameter is enabled, DEBUG() function is defined in header file
    if (DEBUG())
    {
        std::cout << "number of points: " << points->getNumPoints() << std::endl;
    }

    GEO_PrimVDB* vdbPrim = NULL; // empty pointer to vdb primitive

    // iterate over all incoming primitives and find the first one which is VDB
    for (GA_Iterator it(gdp->getPrimitiveRange()); !it.atEnd(); it.advance())
    {
        GEO_Primitive* prim = gdp->getGEOPrimitive(it.getOffset());
        if(dynamic_cast<const GEO_PrimVDB *>(prim))
        {
            vdbPrim = dynamic_cast<GEO_PrimVDB *>(prim);
            break;
        }
    }

    // terminate if volume is not VDB
    if(!vdbPrim)
    {
        addError(SOP_MESSAGE, "First input must contain a VDB!");
        return error();
    }

    // volume primitives in different nodes in Houdini by default share the same volume tree (for memory optimization) this will make sure that we will have our own deep copy of volume tree which we can write to 
    vdbPrim->makeGridUnique();
    
    // get grid base pointer and cast it to float grid pointer
    openvdb::GridBase::Ptr vdbPtrBase = vdbPrim->getGridPtr();
    openvdb::FloatGrid::Ptr vdbPtr = openvdb::gridPtrCast<openvdb::FloatGrid>(vdbPtrBase);

    // get accessor to the float grid
    openvdb::FloatGrid::Accessor vdb_access = vdbPtr->getAccessor();

    // get a reference to transformation of the grid
    const openvdb::math::Transform &vdbGridXform = vdbPtr->transform();

    // loop over all the points by handle
    int i = 0;
    GA_ROHandleV3 Phandle(points->findAttribute(GA_ATTRIB_POINT, "P")); // handle to read only attribute
    GA_Offset ptoff;
    GA_FOR_ALL_PTOFF(points, ptoff)
    {
        // test if user requested abort
        if (progress.wasInterrupted())
            return error();

        // get current pont position
        UT_Vector3 Pvalue = Phandle.get(ptoff);

        // create openvdb vector with values from houdini's vector, transform it from world space to vdb's index space (based on vdb's transformation) and activate voxel at point position
        openvdb::Vec3R p_( Pvalue[0], Pvalue[1], Pvalue[2] );
        openvdb::Coord p_xformed( vdbGridXform.worldToIndexCellCentered(p_) );
        vdb_access.setValueOn( p_xformed );
        
        if (DEBUG())
        {
            std::cout << i << ". point world space position: " << Pvalue << std::endl;
            std::cout << "  volmue index space position: " << p_xformed << std::endl;
        }

        i++;
    }

    return error();
}
