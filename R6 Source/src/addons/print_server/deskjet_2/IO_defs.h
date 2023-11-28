//
//  Copyright (c) 2000, Hewlett-Packard Co.
//  All rights reserved.
//  
//  This software is licensed solely for use with HP products.  Redistribution
//  and use with HP products in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//  
//  -	Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//  -	Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//  -	Neither the name of Hewlett-Packard nor the names of its contributors
//      may be used to endorse or promote products derived from this software
//      without specific prior written permission.
//  -	Redistributors making defect corrections to source code grant to
//      Hewlett-Packard the right to use and redistribute such defect
//      corrections.
//  
//  This software contains technology licensed from third parties; use with
//  non-HP products is at your own risk and may require a royalty.
//  
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
//  CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
//  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
//  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//  DISCLAIMED. IN NO EVENT SHALL HEWLETT-PACKARD OR ITS CONTRIBUTORS
//  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
//  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
//  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
//  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
//  DAMAGE.
//

#define NFAULT_BIT	0x08
#define SELECT_BIT	0x10
#define PERROR_BIT	0x20
#define NACK_BIT	0x40
#define BUSY_BIT	0x80

#define STATUS_MASK (NFAULT_BIT | PERROR_BIT | SELECT_BIT)
#define BUSY_MASK   (BUSY_BIT | NACK_BIT | SELECT_BIT | NFAULT_BIT)

#define DEVICE_IS_BUSY(reg) (reg == BUSY_MASK)
#define DEVICE_IS_OOP(reg)	((reg & STATUS_MASK) == OOP)
#define DJ400_IS_OOP(reg)	((reg & STATUS_MASK) == DJ400_OOP)
#define DJ8XX_IS_OOP(reg)	((reg & STATUS_MASK) == DJ8XX_OOP)
#define DEVICE_JAMMED_OR_TRAPPED(reg) ( ((reg & STATUS_MASK) == JAMMED) || ((reg & STATUS_MASK) == ERROR_TRAP) )
#define DJ8XX_JAMMED_OR_TRAPPED(reg) ( ((reg & STATUS_MASK) == DJ8XX_JAMMED) || ((reg & STATUS_MASK) == ERROR_TRAP) )

#define OOP				(NFAULT_BIT | PERROR_BIT)
#define DJ400_OOP		(NFAULT_BIT | PERROR_BIT | SELECT_BIT)
#define DJ8XX_OOP		(NFAULT_BIT | PERROR_BIT | SELECT_BIT)
#define JAMMED			(PERROR_BIT)
#define DJ8XX_JAMMED	(PERROR_BIT | SELECT_BIT)	// DJ8XX doesn't go offline, 
													// so SELECT set
#define ERROR_TRAP		(0)
#define OFFLINE			(NFAULT_BIT)

#define MAX_BUSY_TIME    30000     // in msec, ie 30 sec
#define C32_STATUS_WAIT  2000      // in msec, ie 2 sec

#define MAX_SLOW_POLL_TIMES		3
#define MIN_XFER_FOR_SLOW_POLL	2
