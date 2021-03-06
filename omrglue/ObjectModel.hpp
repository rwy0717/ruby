/*******************************************************************************
 *
 * (c) Copyright IBM Corp. 1991, 2016
 *
 *  This program and the accompanying materials are made available
 *  under the terms of the Eclipse Public License v1.0 and
 *  Apache License v2.0 which accompanies this distribution.
 *
 *      The Eclipse Public License is available at
 *      http://www.eclipse.org/legal/epl-v10.html
 *
 *      The Apache License v2.0 is available at
 *      http://www.opensource.org/licenses/apache2.0.php
 *
 * Contributors:
 *    Multiple authors (IBM Corp.) - initial implementation and documentation
 *******************************************************************************/

#if !defined(OBJECTMODEL_HPP_)
#define OBJECTMODEL_HPP_

#include "omrcfg.h"
#include "HeapLinkedFreeHeader.hpp"
#include "ModronAssertions.h"
#include "modronbase.h"
#include "objectdescription.h"

#include "gc.h"
#include "Bits.hpp"

class MM_GCExtensionsBase;

#define J9_GC_OBJECT_ALIGNMENT_IN_BYTES 0x8
#define J9_GC_MINIMUM_OBJECT_SIZE 0x10

/**
 * Provides information for a given object.
 * @ingroup GC_Base
 */
class GC_ObjectModel
{
/*
 * Member data and types
 */
private:
	uintptr_t _objectAlignmentInBytes; /**< Cached copy of object alignment for getting object alignment for adjusting for alignment */
	uintptr_t _objectAlignmentShift; /**< Cached copy of object alignment shift, must be log2(_objectAlignmentInBytes)  */
protected:
public:

/*
 * Member functions
 */
private:
protected:
public:
	/**
	 * Initialize the receiver, a new instance of GC_ObjectModel
	 *
	 * @return true on success, false on failure
	 */
	bool
	initialize(MM_GCExtensionsBase *extensions)
	{
		return true;
	}

	void tearDown(MM_GCExtensionsBase *extensions) {}

	MMINLINE uintptr_t
	adjustSizeInBytes(uintptr_t sizeInBytes)
	{
		sizeInBytes =  (sizeInBytes + (J9_GC_OBJECT_ALIGNMENT_IN_BYTES - 1)) & (uintptr_t)~(J9_GC_OBJECT_ALIGNMENT_IN_BYTES - 1);

#if defined(OMR_GC_MINIMUM_OBJECT_SIZE)
		if (sizeInBytes < J9_GC_MINIMUM_OBJECT_SIZE) {
			sizeInBytes = J9_GC_MINIMUM_OBJECT_SIZE;
		}
#endif /* OMR_GC_MINIMUM_OBJECT_SIZE */

		return sizeInBytes;
	}

	MMINLINE uintptr_t
	getSizeInBytesObjectHeader(void)
	{
		return 0;
	}

	/**
	 * Returns TRUE if an object is dead, FALSE otherwise.
	 * @param objectPtr Pointer to an object
	 * @return TRUE if an object is dead, FALSE otherwise
	 */
	MMINLINE bool
	isDeadObject(void *objectPtr)
	{
		return 0 != (*((uintptr_t *)objectPtr) & J9_GC_OBJ_HEAP_HOLE);
	}

	/**
	 * Returns TRUE if an object is a dead single slot object, FALSE otherwise.
	 * @param objectPtr Pointer to an object
	 * @return TRUE if an object is a dead single slot object, FALSE otherwise
	 */
	MMINLINE bool isSingleSlotDeadObject(omrobjectptr_t objectPtr)
	{
		return J9_GC_SINGLE_SLOT_HOLE == (*((uintptr_t *)objectPtr) & J9_GC_OBJ_HEAP_HOLE_MASK);
	}

	/**
	 * Returns the size, in bytes, of a single slot dead object.
	 * @param objectPtr Pointer to an object
	 * @return The size, in bytes, of a single slot dead object
	 */
	MMINLINE uintptr_t getSizeInBytesSingleSlotDeadObject(omrobjectptr_t objectPtr)
	{
		return sizeof(uintptr_t);
	}

	/**
	 * Returns the size, in bytes, of a multi-slot dead object.
	 * @param objectPtr Pointer to an object
	 * @return The size, in bytes, of a multi-slot dead object
	 */
	MMINLINE uintptr_t getSizeInBytesMultiSlotDeadObject(omrobjectptr_t objectPtr)
	{
		return MM_HeapLinkedFreeHeader::getHeapLinkedFreeHeader(objectPtr)->getSize();
	}

	/**
	 * Returns the size in bytes of a dead object.
	 * @param objectPtr Pointer to an object
	 * @return The size in byts of a dead object
	 */
	MMINLINE uintptr_t getSizeInBytesDeadObject(omrobjectptr_t objectPtr)
	{
		if(isSingleSlotDeadObject(objectPtr)) {
			return getSizeInBytesSingleSlotDeadObject(objectPtr);
		}
		return getSizeInBytesMultiSlotDeadObject(objectPtr);
	}

	MMINLINE uintptr_t
	getConsumedSizeInSlotsWithHeader(omrobjectptr_t objectPtr)
	{
		return MM_Bits::convertBytesToSlots(getSizeInBytesWithHeader(objectPtr));
	}

	MMINLINE uintptr_t
	getConsumedSizeInBytesWithHeader(omrobjectptr_t objectPtr)
	{
		return adjustSizeInBytes(getSizeInBytesWithHeader(objectPtr));
	}

	MMINLINE uintptr_t
	getConsumedSizeInBytesWithHeaderForMove(omrobjectptr_t objectPtr)
	{
		return getConsumedSizeInBytesWithHeader(objectPtr);
	}

	MMINLINE uintptr_t
	getSizeInBytesWithHeader(omrobjectptr_t objectPtr)
	{
		return getSizeInBytesWithoutHeader(objectPtr) + getSizeInBytesObjectHeader();
	}

	MMINLINE uintptr_t
	getAdjustedSizeInBytesForOMRBuf(long size)
	{
		return adjustSizeInBytes(getSizeInBytesWithHeaderForOMRBuf(size));
	}

	MMINLINE uintptr_t
	getAdjustedSizeInBytesWithoutOMRBufHeader(long size)
	{
		return adjustSizeInBytes(getSizeInBytesWithHeaderForOMRBuf(size)) - sizeof(OMRBuffer);
	}

	MMINLINE uintptr_t
	getSizeInBytesWithHeaderForOMRBuf(long size)
	{
		return getSizeInBytesWithoutHeaderForOMRBuf(size) + getSizeInBytesObjectHeader();
	}

	MMINLINE uintptr_t
	getSizeInBytesWithoutHeaderForOMRBuf(long size)
	{
		return sizeof(OMRBuffer) + size;
	}

	MMINLINE uintptr_t
	getSizeInBytesWithoutHeader(omrobjectptr_t objectPtr)
	{
		languageobjectptr_t languageObj = convertOmrObjectToLanguageObject(objectPtr);
		RBasicOMR * rubyObj = (RBasicOMR *) languageObj;
		if ((rubyObj->flags & T_MASK) == T_OMRBUF) {
			OMRBuffer *buf = (OMRBuffer *) rubyObj;
			return (uintptr_t)getSizeInBytesWithoutHeaderForOMRBuf(buf->size);
		} else {
			return (uintptr_t)sizeof(RVALUE);
		}
	}

	MMINLINE void
	setObjectSize(omrobjectptr_t objectPtr, uintptr_t size)
	{
		/* Do nothing. In ruby, all objects are uniform sized. We do not write the size into the header. */
	}

	MMINLINE languageobjectptr_t
	convertOmrObjectToLanguageObject(omrobjectptr_t objectPtr)
	{
		uintptr_t p = (uintptr_t) objectPtr;
		return (languageobjectptr_t) (p + getSizeInBytesObjectHeader());
	}

	MMINLINE omrobjectptr_t
	convertLanguageObjectToOmrObject(languageobjectptr_t objectPtr)
	{
		uintptr_t p = (uintptr_t) objectPtr;
		return (languageobjectptr_t) (p - getSizeInBytesObjectHeader());
	}

	MMINLINE void
	preMove(OMR_VMThread* vmThread, omrobjectptr_t objectPtr)
	{
		/* do nothing */
	}

	MMINLINE void
	postMove(OMR_VMThread* vmThread, omrobjectptr_t objectPtr)
	{
		/* do nothing */
	}

	/**
	 * Returns TRUE if an object is remembered, FALSE otherwise.
	 * @param objectPtr Pointer to an object
	 * @return TRUE if an object is remembered, FALSE otherwise
	 */
	MMINLINE bool isRemembered(omrobjectptr_t objectPtr)
	{
		return false;
	}


 	/**
	 * Set run-time Object Alignment in the heap value
	 * Function exists because we can only determine it is way after ObjectModel is init
	 */
	MMINLINE void
	setObjectAlignmentInBytes(uintptr_t objectAlignmentInBytes)
	{
		_objectAlignmentInBytes = objectAlignmentInBytes;
	}

 	/**
	 * Set run-time Object Alignment Shift value
	 * Function exists because we can only determine it is way after ObjectModel is init
	 */
	MMINLINE void
	setObjectAlignmentShift(uintptr_t objectAlignmentShift)
	{
		_objectAlignmentShift = objectAlignmentShift;
	}

 	/**
	 * Get run-time Object Alignment in the heap value
	 */
	MMINLINE uintptr_t
	getObjectAlignmentInBytes()
	{
		return _objectAlignmentInBytes;
	}

 	/**
	 * Get run-time Object Alignment Shift value
	 */
	MMINLINE uintptr_t
	getObjectAlignmentShift()
	{
		return _objectAlignmentShift;
	}
};

#endif /* OBJECTMODEL_HPP_ */
