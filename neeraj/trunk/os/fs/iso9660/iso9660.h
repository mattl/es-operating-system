/*
 * Copyright 2008 Google Inc.
 * Copyright 2006 Nintendo Co., Ltd.
 *  
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

enum
{
    // Volume Descriptor
    VD_Type = 0,
    VD_StandardIdentifier = 1,
    VD_Version = 6,

    VDT_BootRecord = 0,
    VDT_Primary = 1,
    VDT_Supplementary = 2,
    VDT_Partition = 3,
    VDT_Terminator = 255,

    // Primary/Supplementary Volume Descriptor
    VD_SystemIdentifier = 8,
    VD_VolumeIdentifier = 40,
    VD_VolumeSpaceSize = 80,
    VD_EscapeSequences = 88,
    VD_VolumeSetSize = 120,
    VD_VolumeSequenceNumber = 124,
    VD_LogicalBlockSize = 128,
    VD_PathTableSize = 132,
    VD_TypeLPathTable = 140,
    VD_OptionalTypeLPathTable = 144,
    VD_TypeMPathTable = 148,
    VD_OptionalTypeMPathTable = 152,
    VD_RootDirectory = 156,
    VD_VolumeSetIdentifier = 190,
    VD_PublisherIdentifier = 318,
    VD_DataPreparerIdentifier = 446,
    VD_ApplicationIdentifier = 574,
    VD_CopyrightFileIdentifier = 702,
    VD_AbstractFileIdentifier = 739,
    VD_BibliographicFileIdentifier = 776,
    VD_CreationDateAndTime = 813,
    VD_ModificationDateAndTime = 830,
    VD_ExpirationDateAndTime = 847,
    VD_EffectiveDateAndTime = 864,
    VD_FileStructureVersion = 881,

    // Directory Record
    DR_Length = 0,
    DR_AttributeRecordLength = 1,
    DR_Location = 2,
    DR_DataLength = 10,
    DR_RecordingDateAndTime = 18,
    DR_FileFlags = 25,
    DR_FileUnitSize = 26,
    DR_InterleaveGapSize = 27,
    DR_VolumeSequenceNumber = 28,
    DR_FileIdentifierLength = 32,
    DR_FileIdentifier = 33,

    // Date and Time
    DT_Year = 0,
    DT_Month = 1,
    DT_Day = 2,
    DT_Hour = 3,
    DT_Minute = 4,
    DT_Second = 5,
    DT_Offset = 6,

    // File Flags
    FF_Existence = 0x01,
    FF_Directory = 0x02,
    FF_AssociatedFile = 0x04,
    FF_Record = 0x08,
    FF_Protection = 0x10,
    FF_MultiExtent = 0x80
};
