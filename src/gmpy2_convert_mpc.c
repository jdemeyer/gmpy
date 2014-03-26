/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * gmpy2_convert_mpc.c                                                     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Python interface to the GMP or MPIR, MPFR, and MPC multiple precision   *
 * libraries.                                                              *
 *                                                                         *
 * Copyright 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007,               *
 *           2008, 2009 Alex Martelli                                      *
 *                                                                         *
 * Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014 Case Van Horsen      *
 *                                                                         *
 * This file is part of GMPY2.                                             *
 *                                                                         *
 * GMPY2 is free software: you can redistribute it and/or modify it under  *
 * the terms of the GNU Lesser General Public License as published by the  *
 * Free Software Foundation, either version 3 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * GMPY2 is distributed in the hope that it will be useful, but WITHOUT    *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or   *
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public    *
 * License for more details.                                               *
 *                                                                         *
 * You should have received a copy of the GNU Lesser General Public        *
 * License along with GMPY2; if not, see <http://www.gnu.org/licenses/>    *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Please see the documentation for GMPy_MPFR_From_MPFR for detailed
 * description of this function.
 */


static MPC_Object *
GMPy_MPC_From_MPC(MPC_Object *obj, mpfr_prec_t rprec, mpfr_prec_t iprec,
                  CTXT_Object *context)
{
    MPC_Object *result = NULL;
    mpfr_prec_t tempr = 0, tempi = 0;
    int rr, ri, dr, di;

    assert(MPC_Check(obj));

    CHECK_CONTEXT_SET_EXPONENT(context);

    if (rprec == 0)
        rprec = GET_REAL_PREC(context);
    else if (rprec == 1)
        rprec = mpfr_get_prec(mpc_realref(obj->c));

    if (iprec == 0)
        iprec = GET_IMAG_PREC(context);
    else if (iprec == 1)
        iprec = mpfr_get_prec(mpc_imagref(obj->c));


    if (MPC_CheckAndExp(obj)) {
        if (rprec == mpfr_get_prec(mpc_realref(obj->c)) &&
            iprec == mpfr_get_prec(mpc_imagref(obj->c))) {
            Py_INCREF((PyObject*)obj);
            return obj;
        }
        else {
            if ((result = GMPy_MPC_New(rprec, iprec, context))) {
                result->rc = mpc_set(result->c, obj->c, GET_MPC_ROUND(context));
                /* Expanded version of MPC_CLEANUP_2 macro without the check for NAN.
                 */
                MPC_SUBNORMALIZE_2(result, context);
                do {
                    int rcr, rci;
                    rcr = MPC_INEX_RE(result->rc);
                    rci = MPC_INEX_IM(result->rc);
                    if ((rcr && mpfr_zero_p(mpc_realref(MPC(result)))) || (rci && mpfr_zero_p(mpc_imagref(MPC(result))))) {
                        context->ctx.underflow = 1;
                        if (context->ctx.traps & TRAP_UNDERFLOW) {
                            GMPY_OVERFLOW("mpc() underflow");
                            Py_DECREF((PyObject*)result);
                            return NULL;
                        }
                    }
                    if ((rcr && mpfr_inf_p(mpc_realref(MPC(result)))) || (rci && mpfr_inf_p(mpc_imagref(MPC(result))))) {
                        context->ctx.overflow = 1;
                        if (context->ctx.traps & TRAP_OVERFLOW) {
                            GMPY_OVERFLOW("mpc() overflow");
                            Py_DECREF((PyObject*)result);
                            return NULL;
                        }
                    }
                } while(0);
                if (result->rc) {
                    context->ctx.inexact = 1;
                    if (context->ctx.traps & TRAP_INEXACT) {
                        GMPY_INEXACT("mpc() inexact result");
                        Py_DECREF((PyObject*)result);
                        return NULL;
                    }
                }
            }
            return result;
        }
    }


    if (context->ctx.traps & TRAP_EXPBOUND) {
        GMPY_EXPBOUND("exponent of existing mpc incompatible with current context");
        return NULL;
    }

    mpc_get_prec2(&tempr, &tempi, obj->c);
    rr = MPC_INEX_RE(obj->rc);
    ri = MPC_INEX_IM(obj->rc);
    dr = MPC_RND_RE(obj->round_mode);
    di = MPC_RND_IM(obj->round_mode);

    if ((result = GMPy_MPC_New(tempr, tempi, context))) {
        /* First make the exponent valid. */
        mpc_set(result->c, obj->c, GET_MPC_ROUND(context));
        rr = mpfr_check_range(mpc_realref(result->c), rr, dr);
        ri = mpfr_check_range(mpc_imagref(result->c), ri, di);
        /* Round to the desired precision. */
        result->rc = MPC_INEX(rr, ri);
    }

    return result;
}

static MPC_Object *
GMPy_MPC_From_PyComplex(PyObject *obj, mpfr_prec_t rprec, mpfr_prec_t iprec,
                        CTXT_Object *context)
{
    MPC_Object *result;

    assert(PyComplex_Check(obj));

    CHECK_CONTEXT_SET_EXPONENT(context);

    if (rprec == 0)
        rprec = GET_REAL_PREC(context);
    else if (rprec == 1)
        rprec = DBL_MANT_DIG;

    if (iprec == 0)
        iprec = GET_IMAG_PREC(context);
    else if (iprec == 1)
        rprec = DBL_MANT_DIG;

    if ((result = GMPy_MPC_New(rprec, iprec, context)))
        mpc_set_d_d(result->c, PyComplex_RealAsDouble(obj),
                    PyComplex_ImagAsDouble(obj), GET_MPC_ROUND(context));

    MPC_SUBNORMALIZE_2(result, context);
    /* Need to do exception checks! */

    return result;
}

static MPC_Object *
GMPy_MPC_From_MPFR(MPFR_Object *obj, mpfr_prec_t rprec, mpfr_prec_t iprec,
                   CTXT_Object *context)
{
    MPC_Object *result;

    CHECK_CONTEXT_SET_EXPONENT(context);

    if (rprec == 0)
        rprec = GET_REAL_PREC(context);
    else if (rprec == 1)
        rprec = mpfr_get_prec(obj->f);

    if (iprec == 0)
        iprec = GET_IMAG_PREC(context);
    else if (iprec == 1)
        rprec = mpfr_get_prec(obj->f);

    if ((result = GMPy_MPC_New(rprec, iprec, context)))
        result->rc = mpc_set_fr(result->c, obj->f, GET_MPC_ROUND(context));

    MPC_SUBNORMALIZE_2(result, context);
    /* Need to do exception checks! */

    return result;
}

static MPC_Object *
GMPy_MPC_From_PyFloat(PyObject *obj, mpfr_prec_t rprec, mpfr_prec_t iprec,
                   CTXT_Object *context)
{
    MPC_Object *result;

    CHECK_CONTEXT_SET_EXPONENT(context);

    assert(PyFloat_Check(obj));

    if (rprec == 0)
        rprec = GET_REAL_PREC(context);
    else if (rprec == 1)
        rprec = DBL_MANT_DIG;

    if (iprec == 0)
        iprec = GET_IMAG_PREC(context);
    else if (iprec == 1)
        rprec = DBL_MANT_DIG;

    if ((result = GMPy_MPC_New(rprec, iprec, context)))
        result->rc = mpc_set_d(result->c, PyFloat_AS_DOUBLE(obj),
                               GET_MPC_ROUND(context));

    MPC_SUBNORMALIZE_2(result, context);
    /* Need to do exception checks! */

    return result;
}

static MPC_Object *
GMPy_MPC_From_MPZ(MPZ_Object *obj, mpfr_prec_t rprec, mpfr_prec_t iprec,
                  CTXT_Object *context)
{
    MPC_Object *result = NULL;

    assert(MPZ_Check(obj));

    CHECK_CONTEXT_SET_EXPONENT(context);

    if (rprec == 0 || rprec == 1)
        rprec = GET_REAL_PREC(context) + rprec * GET_GUARD_BITS(context);

    if (iprec == 0 || iprec == 1)
        iprec = GET_IMAG_PREC(context) + iprec * GET_GUARD_BITS(context);

    if ((result = GMPy_MPC_New(rprec, iprec, context)))
        result->rc = mpc_set_z(result->c, obj->z, GET_MPC_ROUND(context));

    MPC_SUBNORMALIZE_2(result, context);
    /* Need to do exception checks! */

    return result;
}

static MPC_Object *
GMPy_MPC_From_MPQ(MPQ_Object *obj, mpfr_prec_t rprec, mpfr_prec_t iprec,
                  CTXT_Object *context)
{
    MPC_Object *result = NULL;

    assert(MPQ_Check(obj));

    CHECK_CONTEXT_SET_EXPONENT(context);

    if (rprec == 0 || rprec == 1)
        rprec = GET_REAL_PREC(context) + rprec * GET_GUARD_BITS(context);

    if (iprec == 0 || iprec == 1)
        iprec = GET_IMAG_PREC(context) + iprec * GET_GUARD_BITS(context);

    if ((result = GMPy_MPC_New(rprec, iprec, context)))
        result->rc = mpc_set_q(result->c, obj->q, GET_MPC_ROUND(context));

    MPC_SUBNORMALIZE_2(result, context);
    /* Need to do exception checks! */

    return result;
}

static MPC_Object *
GMPy_MPC_From_Fraction(PyObject *obj, mpfr_prec_t rprec, mpfr_prec_t iprec,
                       CTXT_Object *context)
{
    MPC_Object *result = NULL;
    MPQ_Object *tempq;

    assert(IS_RATIONAL(obj));

    CHECK_CONTEXT_SET_EXPONENT(context);

    if ((tempq = GMPy_MPQ_From_Fraction(obj, context))) {
        result = GMPy_MPC_From_MPQ(tempq, rprec, iprec, context);
        Py_DECREF((PyObject*)tempq);
    }

    MPC_SUBNORMALIZE_2(result, context);
    /* Need to do exception checks! */

    return result;
}

static MPC_Object *
GMPy_MPC_From_Decimal(PyObject *obj, mpfr_prec_t rprec, mpfr_prec_t iprec,
                      CTXT_Object *context)
{
    MPC_Object *result = NULL;
    MPFR_Object *tempf;
    mpfr_prec_t oldmpfr, oldreal;
    int oldmpfr_round, oldreal_round;

    oldmpfr = GET_MPFR_PREC(context);
    oldreal = GET_REAL_PREC(context);
    oldmpfr_round = GET_MPFR_ROUND(context);
    oldreal_round = GET_REAL_ROUND(context);

    assert(IS_DECIMAL(obj));

    CHECK_CONTEXT_SET_EXPONENT(context);

    context->ctx.mpfr_prec = oldreal;
    context->ctx.mpfr_round = oldreal_round;

    tempf = GMPy_MPFR_From_Decimal(obj, rprec, context);

    context->ctx.mpfr_prec = oldmpfr;
    context->ctx.mpfr_round = oldmpfr_round;

    result = GMPy_MPC_New(0, 0, context);
    if (!tempf || !result) {
        Py_XDECREF((PyObject*)tempf);
        Py_XDECREF((PyObject*)result);
        return NULL;
    }

    result->rc = MPC_INEX(tempf->rc, 0);
    mpfr_swap(mpc_realref(result->c), tempf->f);
    Py_DECREF(tempf);

    MPC_SUBNORMALIZE_2(result, context);
    /* Need to do exception checks! */

    return result;
}

static MPC_Object *
GMPy_MPC_From_PyIntOrLong(PyObject *obj, mpfr_prec_t rprec, mpfr_prec_t iprec,
                          CTXT_Object *context)
{
    MPC_Object *result = NULL;
    MPZ_Object *tempz;

    assert(PyIntOrLong_Check(obj));

    CHECK_CONTEXT_SET_EXPONENT(context);

    if ((tempz = GMPy_MPZ_From_PyIntOrLong(obj, context))) {
        result = GMPy_MPC_From_MPZ(tempz, rprec, iprec, context);
        Py_DECREF((PyObject*)tempz);
    }

    return result;
}

/* Python's string representation of a complex number differs from the format
 * used by MPC. Both MPC and Python surround the complex number with '(' and
 * ')' but Python adds a 'j' after the imaginary component and MPC requires a
 * space between the real and imaginery components. GMPy_MPC_From_PyStr tries
 * to work around the differences as by reading two MPFR-compatible numbers
 * from the string and storing into the real and imaginary components
 * respectively.
 */

static MPC_Object *
GMPy_MPC_From_PyStr(PyObject *s, int base, mpfr_prec_t rprec, mpfr_prec_t iprec,
                    CTXT_Object *context)
{
    MPC_Object *result;
    PyObject *ascii_str = NULL;
    Py_ssize_t len;
    char *cp, *unwind, *tempchar, *lastchar;
    int firstp = 0, lastp = 0, real_rc = 0, imag_rc = 0;

    CHECK_CONTEXT_SET_EXPONENT(context);

    if (PyBytes_Check(s)) {
        len = PyBytes_Size(s);
        cp = (char*)PyBytes_AsString(s);
    }
    else if (PyUnicode_Check(s)) {
        ascii_str = PyUnicode_AsASCIIString(s);
        if (!ascii_str) {
            VALUE_ERROR("string contains non-ASCII characters");
            return NULL;
        }
        len = PyBytes_Size(ascii_str);
        cp = (char*)PyBytes_AsString(ascii_str);
    }
    else {
        TYPE_ERROR("string required");
        return NULL;
    }

    /* Don't allow NULL characters */
    if (strlen(cp) != len) {
        VALUE_ERROR("string without NULL characters expected");
        Py_XDECREF(ascii_str);
        return NULL;
    }

    if (!(result = GMPy_MPC_New(rprec, iprec, context))) {
        Py_XDECREF(ascii_str);
        return NULL;
    }

    /* Get a pointer to the last valid character (ignoring trailing
     * whitespace.) */
    lastchar = cp + len - 1;
    while (isspace(*lastchar))
        lastchar--;

    /* Skip trailing ). */
    if (*lastchar == ')') {
        lastp = 1;
        lastchar--;
    }

    /* Skip trailing j. */
    if (*lastchar == 'j')
        lastchar--;

    /* Skip leading whitespace. */
    while (isspace(*cp))
        cp++;

    /* Skip a leading (. */
    if (*cp == '(') {
        firstp = 1;
        cp++;
    }

    if (firstp != lastp) goto invalid_string;

    /* Read the real component first. */
    unwind = cp;
    real_rc = mpfr_strtofr(mpc_realref(result->c), cp, &tempchar, base,
                           GET_REAL_ROUND(context));

    /* Verify that at least one valid character was read. */
    if (cp == tempchar) goto invalid_string;

    /* If the next character is a j, then the real component is 0 and
     * we just read the imaginary componenet.
     */
    if (*tempchar == 'j') {
        mpfr_set_zero(mpc_realref(result->c), MPFR_RNDN);
        cp = unwind;
    }
    else {
        /* Read the imaginary component next. */
        cp = tempchar;
    }

    imag_rc = mpfr_strtofr(mpc_imagref(result->c), cp, &tempchar, base,
                           GET_IMAG_ROUND(context));

    if (cp == tempchar && tempchar > lastchar)
        goto valid_string;

    if (*tempchar != 'j' && *cp != ' ')
        goto invalid_string;

    if (tempchar <= lastchar)
        goto invalid_string;

  valid_string:
    Py_XDECREF(ascii_str);
    result->rc = MPC_INEX(real_rc, imag_rc);

    /* Expanded version of MPC_CLEANUP_2 macro without the check for NAN.
     */
    MPC_SUBNORMALIZE_2(result, context);
    if (MPC_IS_NAN_P(result)) {
        context->ctx.invalid = 1;
        if (context->ctx.traps & TRAP_INVALID) {
            GMPY_INVALID("mpc() invalid operation");
            Py_DECREF((PyObject*)result);
            return NULL;
        }
    }
    do {
        int rcr, rci;
        rcr = MPC_INEX_RE(result->rc);
        rci = MPC_INEX_IM(result->rc);
        if ((rcr && mpfr_zero_p(mpc_realref(MPC(result)))) || (rci && mpfr_zero_p(mpc_imagref(MPC(result))))) {
            context->ctx.underflow = 1;
            if (context->ctx.traps & TRAP_UNDERFLOW) {
                GMPY_OVERFLOW("mpc() underflow");
                Py_DECREF((PyObject*)result);
                return NULL;
            }
        }
        if ((rcr && mpfr_inf_p(mpc_realref(MPC(result)))) || (rci && mpfr_inf_p(mpc_imagref(MPC(result))))) {
            context->ctx.overflow = 1;
            if (context->ctx.traps & TRAP_OVERFLOW) {
                GMPY_OVERFLOW("mpc() overflow");
                Py_DECREF((PyObject*)result);
                return NULL;
            }
        }
    } while(0);
    if (result->rc) {
        context->ctx.inexact = 1;
        if (context->ctx.traps & TRAP_INEXACT) {
            GMPY_INEXACT("mpc() inexact result");
            Py_DECREF((PyObject*)result);
            return NULL;
        }
    }

    return result;

  invalid_string:
    VALUE_ERROR("invalid string in mpc()");
    Py_DECREF((PyObject*)result);
    Py_XDECREF(ascii_str);
    return NULL;
}

/* See the comments for GMPy_MPFR_From_Real_Temp. */

static MPC_Object *
GMPy_MPC_From_Complex(PyObject* obj, mp_prec_t rprec, mp_prec_t iprec,
                           CTXT_Object *context)
{
    CHECK_CONTEXT_SET_EXPONENT(context);

    if (MPC_Check(obj))
        return GMPy_MPC_From_MPC((MPC_Object*)obj, rprec, iprec, context);

    if (MPFR_Check(obj))
        return GMPy_MPC_From_MPFR((MPFR_Object*)obj,
                                  mpfr_get_prec(MPFR(obj)),
                                  mpfr_get_prec(MPFR(obj)),
                                  context
                                 );

    if (PyFloat_Check(obj))
        return GMPy_MPC_From_PyFloat(obj, 53, 53, context);

    if (PyComplex_Check(obj))
        return GMPy_MPC_From_PyComplex(obj, 53, 53, context);

    if (MPQ_Check(obj))
        return GMPy_MPC_From_MPQ((MPQ_Object*)obj, rprec, iprec, context);

    if (MPZ_Check(obj) || XMPZ_Check(obj))
        return GMPy_MPC_From_MPZ((MPZ_Object*)obj, rprec, iprec, context);

    if (PyIntOrLong_Check(obj))
        return GMPy_MPC_From_PyIntOrLong(obj, rprec, iprec, context);

    if (IS_DECIMAL(obj))
        return GMPy_MPC_From_Decimal(obj, rprec, iprec, context);

    if (IS_FRACTION(obj))
        return GMPy_MPC_From_Fraction(obj, rprec, iprec, context);

    TYPE_ERROR("object could not be converted to 'mpc'");
    return NULL;
}

static PyObject *
GMPy_PyStr_From_MPC(MPC_Object *self, int base, int digits, CTXT_Object *context)
{
    PyObject *tempreal = 0, *tempimag = 0, *result;

    CHECK_CONTEXT_SET_EXPONENT(context);

    if (!((base >= 2) && (base <= 62))) {
        VALUE_ERROR("base must be in the interval 2 ... 62");
        return NULL;
    }
    if ((digits < 0) || (digits == 1)) {
        VALUE_ERROR("digits must be 0 or >= 2");
        return NULL;
    }

    tempreal = mpfr_ascii(mpc_realref(self->c), base, digits,
                          MPC_RND_RE(GET_MPC_ROUND(context)));
    tempimag = mpfr_ascii(mpc_imagref(self->c), base, digits,
                          MPC_RND_IM(GET_MPC_ROUND(context)));

    if (!tempreal || !tempimag) {
        Py_XDECREF(tempreal);
        Py_XDECREF(tempimag);
        return NULL;
    }

    result = Py_BuildValue("(NN)", tempreal, tempimag);
    if (!result) {
        Py_DECREF(tempreal);
        Py_DECREF(tempimag);
    }
    return result;
}

static PyObject *
GMPy_MPC_Float_Slot(PyObject *self)
{
    TYPE_ERROR("can't covert 'mpc' to 'float'");
    return NULL;
}

PyDoc_STRVAR(doc_mpc_complex, "Convert 'mpc' to 'complex'.");

static PyObject *
GMPy_PyComplex_From_MPC(PyObject *self, PyObject *other)
{
    CTXT_Object *context = NULL;
    double real, imag;

    CHECK_CONTEXT_SET_EXPONENT(context);

    real = mpfr_get_d(mpc_realref(MPC(self)), GET_REAL_ROUND(context));
    imag = mpfr_get_d(mpc_imagref(MPC(self)), GET_IMAG_ROUND(context));

    return PyComplex_FromDoubles(real, imag);
}

static PyObject *
GMPy_MPC_Long_Slot(PyObject *self)
{
    TYPE_ERROR("can't covert mpc to long");
    return NULL;
}

static PyObject *
GMPy_MPC_Int_Slot(PyObject *self)
{
    TYPE_ERROR("can't covert mpc to int");
    return NULL;
}

/*
 * coerce any number to a mpc
 */

int
GMPy_MPC_convert_arg(PyObject *arg, PyObject **ptr)
{
    MPC_Object *newob = GMPy_MPC_From_Complex(arg, 0, 0, NULL);

    if (newob) {
        *ptr = (PyObject*)newob;
        return 1;
    }
    else {
        TYPE_ERROR("can't convert argument to 'mpc'");
        return 0;
    }
}

/* str and repr implementations for mpc */
static PyObject *
GMPy_MPC_Str_Slot(MPC_Object *self)
{
    PyObject *result, *temp;
    mpfr_prec_t rbits, ibits;
    long rprec, iprec;
    char fmtstr[30];

    mpc_get_prec2(&rbits, &ibits, MPC(self));
    rprec = (long)(log10(2) * (double)rbits) + 2;
    iprec = (long)(log10(2) * (double)ibits) + 2;

    sprintf(fmtstr, "{0:.%ld.%ldg}", rprec, iprec);

    temp = Py_BuildValue("s", fmtstr);
    if (!temp)
        return NULL;
    result = PyObject_CallMethod(temp, "format", "O", self);
    Py_DECREF(temp);
    return result;
}

static PyObject *
GMPy_MPC_Repr_Slot(MPC_Object *self)
{
    PyObject *result, *temp;
    mpfr_prec_t rbits, ibits;
    long rprec, iprec;
    char fmtstr[30];

    mpc_get_prec2(&rbits, &ibits, MPC(self));
    rprec = (long)(log10(2) * (double)rbits) + 2;
    iprec = (long)(log10(2) * (double)ibits) + 2;

    if (rbits != DBL_MANT_DIG || ibits !=DBL_MANT_DIG)
        sprintf(fmtstr, "mpc('{0:.%ld.%ldg}',(%ld,%ld))",
                rprec, iprec, rbits, ibits);
    else
        sprintf(fmtstr, "mpc('{0:.%ld.%ldg}')", rprec, iprec);

    temp = Py_BuildValue("s", fmtstr);
    if (!temp)
        return NULL;
    result = PyObject_CallMethod(temp, "format", "O", self);
    Py_DECREF(temp);
    return result;
}



