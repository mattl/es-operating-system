/*
 * Copyright 2008 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
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

#ifndef NINTENDO_ES_KERNEL_I386_MPS_H_INCLUDED
#define NINTENDO_ES_KERNEL_I386_MPS_H_INCLUDED

#include <es.h>
#include <string.h>

class Mps
{
public:
    class Visitor;

    /** MP floating pointer structure
     */
    struct FloatingPointerStructure
    {
        char signature[4];  // _MP_
        u32  address;
        u8   length;
        u8   specRev;
        u8   sum;
        u8   featureBytes[5];

        void print() const
        {
            esReport("%4.4s\n", signature);
            esReport("Configuration Table: %08x\n", address);
            esReport("Length: %d paragraph\n", length);
            esReport("Version %d.%d\n", 1 + specRev / 10, specRev % 10);
            esReport("System Configuration Type: %d\n", featureBytes[0]);
            esReport("IMCRP: %d\n", (featureBytes[1] & 0x80) ? 1 : 0);
        }
    };

    /** MP configuration table header
     */
    struct ConfigurationTableHeader
    {
        char signature[4];  // PCMP
        u16  baseTableLength;
        u8   specRev;
        u8   sum;
        char oemString[8];
        char productIDString[12];
        u32  oemTablePointer;
        u16  oemTableSize;
        u16  entryCount;
        u32  localApicAddress;
        u16  extendedTableLength;
        u8   extendedTableSum;
        u8   reserved;

        bool accept(Visitor& visitor);

        void print() const
        {
            esReport("%4.4s\n", signature);
            esReport("Length: %d bytes\n", baseTableLength);
            esReport("Spec Revison: %x\n", specRev);
            esReport("OEM ID: %8.8s\n", oemString);
            esReport("PRODUCT ID: %12.12s\n", productIDString);
            esReport("Entry Count: %d\n", entryCount);
            esReport("Local APIC Address: %x\n", localApicAddress);
        }
    };

    /* Base MP configuration table entry types
     */
    enum
    {
        PROCESSOR,
        BUS,
        IO_APIC,
        IO_INTERRUPT_ASSIGNMENT,
        LOCAL_INTERRUPT_ASSIGNMENT
    };

    struct Processor
    {
        u8  entryType;      // PROCESSOR
        u8  id;             // Local APIC ID
        u8  version;
        u8  flags;
        u32 signature;
        u32 featureFlags;
        u32 reserved[2];

        bool isUsable() const
        {
            return (flags & 1) ? true : false;
        }

        bool isBootstrapProcessor() const
        {
            return (flags & 2) ? true : false;
        }

        void print() const
        {
            esReport("PROCESSOR:\n");
            esReport("\tLocal APIC ID: %u\n", id);
            esReport("\tLocal APIC VERSION: %d\n", version);
            esReport("\tCPU FLAGS: EN: %d\n", isUsable());
            esReport("\tCPU FLAGS: BP: %d\n", isBootstrapProcessor());
            esReport("\tCPU STEPPING: %d\n", signature & 0x0f);
            esReport("\tCPU MODEL: %d\n", (signature >> 4) & 0x0f);
            esReport("\tCPU FAMILY: %d\n", (signature >> 8) & 0x0f);
            esReport("\tFEATURE FLAGS: %x\n", featureFlags);
        }
    };

    struct Bus
    {
        u8   entryType;     // BUS
        u8   id;            // Bus ID
        char typeString[6]; __attribute__ ((packed));

        void print() const
        {
            esReport("BUS:\n");
            esReport("\tBUS ID: %u\n", id);
            esReport("\tBUS TYPE: %6.6s\n", typeString);
        }
    };

    struct IOApic
    {
        u8  entryType;      // IO_APIC
        u8  id;             // I/O APIC ID
        u8  version;
        u8  flags;
        u32 address;        // Memory-mapped address of I/O APIC

        bool isUsable() const
        {
            return (flags & 1) ? true : false;
        }

        void print() const
        {
            esReport("I/O APIC:\n");
            esReport("\tI/O APIC ID: %u\n", id);
            esReport("\tI/O APIC VERSION: %d\n", version);
            esReport("\tI/O APIC FLAGS: EN: %d\n", isUsable());
            esReport("\tI/O APIC Address: %x\n", address);
        }
    };

    struct InterruptAssignment
    {
        u8  entryType;      // IO_INTERRUPT_ASSIGNMENT
        u8  type;
        u16 flags;
        u8  busID;
        u8  busIRQ;
        u8  apicID;
        u8  apicINTINn;

        u8 getPolarity() const
        {
            return flags & 0x03;
        }

        u8 getTriggerMode() const
        {
            return (flags >> 2) & 0x03;
        }

        void print() const
        {
            esReport("I/O Interrupt Assignment:\n");
            esReport("\tInterrupt Type: %d\n", type);
            esReport("\tPO: %d\n", getPolarity());
            esReport("\tEL: %d\n", getTriggerMode());
            esReport("\tSource BUS ID: %u\n", busID);
            esReport("\tSource BUS IRQ: %d\n", busIRQ);
            esReport("\tDestination I/O APIC ID: %u\n", apicID);
            esReport("\tDestination I/O APIC INTIN#: %d\n", apicINTINn);
        }
    };

    struct LocalInterruptAssignment
    {
        u8  entryType;      // LOCAL_INTERRUPT_ASSIGNMENT
        u8  type;
        u16 flags;
        u8  busID;
        u8  busIRQ;
        u8  apicID;
        u8  apicINTINn;

        u8 getPolarity() const
        {
            return flags & 0x03;
        }

        u8 getTriggerMode() const
        {
            return (flags >> 2) & 0x03;
        }

        void print() const
        {
            esReport("Local Interrupt Assignment:\n");
            esReport("\tInterrupt Type: %d\n", type);
            esReport("\tPO: %d\n", getPolarity());
            esReport("\tEL: %d\n", getTriggerMode());
            esReport("\tSource BUS ID: %u\n", busID);
            esReport("\tSource BUS IRQ: %d\n", busIRQ);
            esReport("\tDestination Local APIC ID: %u\n", apicID);
            esReport("\tDestination Local APIC INTIN#: %d\n", apicINTINn);
        }
    };

    class Visitor
    {
    public:
        virtual bool at(const Processor* processor)
        {
            return true;
        }
        virtual bool at(const Bus* bus)
        {
            return true;
        }
        virtual bool at(const IOApic* ioApic)
        {
            return true;
        }
        virtual bool at(const InterruptAssignment* interrupt)
        {
            return true;
        }
        virtual bool at(const LocalInterruptAssignment* interrupt)
        {
            return true;
        }
    };

    class ProcessorCount : public Visitor
    {
        int processorCount;
        u8  isaBus;
    public:
        ProcessorCount(int count = 0) :
            processorCount(count),
            isaBus(0)
        {
        }
        bool at(const Processor* processor)
        {
            if (processor->isUsable())
            {
                ++processorCount;
            }
            return true;
        }
        bool at(const Bus* bus)
        {
            if (memcmp(bus->typeString, "ISA   ", 6) == 0)
            {
                isaBus = bus->id;
            }
            return true;
        }
        operator int() const
        {
            return processorCount;
        }
        u8 getISABusID() const
        {
            return isaBus;
        }
    };

    class Printer : public Visitor
    {
    public:
        bool at(const Processor* processor)
        {
            processor->print();
            return true;
        }
        bool at(const Bus* bus)
        {
            bus->print();
            return true;
        }
        bool at(const IOApic* ioApic)
        {
            ioApic->print();
            return true;
        }
        bool at(const InterruptAssignment* interrupt)
        {
            interrupt->print();
            return true;
        }
        bool at(const LocalInterruptAssignment* interrupt)
        {
            interrupt->print();
            return true;
        }
    };

    class LookupAssignment : public Visitor
    {
        unsigned int bus;
        unsigned int irq;
        const InterruptAssignment* assignment;
    public:
        LookupAssignment(unsigned int bus, unsigned int irq) :
            bus(bus),
            irq(irq),
            assignment(0)
        {
        }
        bool at(const InterruptAssignment* interrupt)
        {
            if (interrupt->type == 0 && // INT
                interrupt->busID == bus &&
                interrupt->busIRQ == irq)
            {
                assignment = interrupt;
                return false;
            }
            return true;
        }
        operator const InterruptAssignment*() const
        {
            return assignment;
        }
        const InterruptAssignment* operator->() const
        {
            return assignment;
        }
    };

    class LookupIoApic : public Visitor
    {
        u8 apicID;
        const IOApic* apic;
    public:
        LookupIoApic(u8 apicID) :
            apicID(apicID),
            apic(0)
        {
        }
        bool at(const IOApic* ioApic)
        {
            if (ioApic->id == apicID)
            {
                apic = ioApic;
                return false;
            }
            return true;
        }
        operator const IOApic*() const
        {
            return apic;
        }
        const IOApic* operator->() const
        {
            return apic;
        }
    };

private:
    FloatingPointerStructure* fps;
    ConfigurationTableHeader* cth;
    ProcessorCount processorCount;

    FloatingPointerStructure* lookup(void* base, int len);
    FloatingPointerStructure* lookup();

public:
    Mps();
    FloatingPointerStructure* getFloatingPointerStructure() const
    {
        return fps;
    }
    ConfigurationTableHeader* getConfigurationTableHeader() const
    {
        return cth;
    }

    int getProcessorCount() const
    {
        return processorCount;
    }

    u8 getISABusID() const
    {
        return processorCount.getISABusID();
    }

    /** Lookup interrupt assignment entry for the irq.
     * @return memory mapped I/O APIC address or zero if not found
     */
    volatile u32* getInterruptAssignment(unsigned int bus,
                                         unsigned int irq,
                                         InterruptAssignment& assignment);

    bool accept(Visitor& visitor)
    {
        if (!cth)
        {
            return false;
        }
        cth->accept(visitor);
    }
};

#endif  // NINTENDO_ES_KERNEL_I386_MPS_H_INCLUDED
