/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */
#ifndef PVMF_SM_FSP_REGISTRY_H
#define PVMF_SM_FSP_REGISTRY_H

#ifndef PVMF_SM_FSP_REGISTRY_INTERFACE_H_INCLUDED
#include "pvmf_sm_fsp_registry_interface.h"
#endif

class PVMFSMFSPRegistryPopulatorInterface;
class OsclSharedLibrary;

// CLASS DECLARATION
/**
 * PVMFSMFSPRegistry maintains a list of fsps available which is queryable.
 * The utility also allows the fsp specified by PVUuid to be created and returned
 **/
class PVMFSMFSPRegistry : public PVMFFSPRegistryInterface
{
    public:
        /**
         * Object Constructor function
         **/
        PVMFSMFSPRegistry();

        /**
         * The QueryRegistry for PVMFSMFSPRegistry. Used mainly for Seaching of the UUID
         * whether it is available or not & returns Success if it is found else failure.
         *
         * @param aInputType Input Format Type
         *
         * @param aOutputType Output Format Type
         *
         * @param aUuids Reference to the UUID registered
         *
         * @returns Success or Failure
         **/
        virtual PVMFStatus QueryRegistry(PVMFFormatType& aInputType, Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids);

        /**
         * The CreateSMFSP for PVMFSMFSPRegistry. Used mainly for creating a SMFSP.
         *
         * @param aUuid UUID returned by the QueryRegistry
         *
         * @returns a pointer to SMFSP
         **/
        virtual PVMFSMFSPBaseNode* CreateSMFSP(PVUuid& aUuid);

        /**
         * The ReleaseSMFSP for PVMFSMFSPRegistry. Used for releasing a SMFSP.
         *
         * @param aUuid UUID recorded at the time of creation of the SMFSP.
         *
         * @param Pointer to the SMFSP to be released
         *
         * @returns True or False
         **/
        virtual bool ReleaseSMFSP(PVUuid& aUuid, PVMFSMFSPBaseNode *aSMFSP);

        /**
         * The RegisterSMFSP for PVMFSMFSPRegistry. Used for registering SMFSPs through the SMFSPInfo object.
         *
         * @param aSMFSPInfo SMFSPInfo object passed to the regisry class. This contains all SMFSPs that need to be registered.
         *
         **/
        virtual void RegisterSMFSP(const PVMFSMFSPInfo& aSMFSPInfo)
        {
            iType.push_back(aSMFSPInfo);
        };

        /**
         * Object destructor function
         **/
        virtual ~PVMFSMFSPRegistry();

    private:
        void AddLoadableModules();
        void RemoveLoadableModules();
        bool CheckPluginAvailability(PVMFFormatType& aInputType, Oscl_Vector<PVUuid, OsclMemAllocator>& aUuids, uint32 aIndex = 0);

        struct PVSMFSPSharedLibInfo
        {
            OsclSharedLibrary* iLib;
            PVMFSMFSPRegistryPopulatorInterface* iFSPLibIfacePtr;
            OsclAny* iContext;
        };

        Oscl_Vector<struct PVSMFSPSharedLibInfo*, OsclMemAllocator> iFSPLibInfoList;
        bool    iMayLoadPluginsDynamically;
        Oscl_Vector<PVMFSMFSPInfo, OsclMemAllocator> iType;
};
#endif
