package com.example.realms
import android.content.Context
import android.graphics.*
import android.view.View

class MaterialBlockView(context: Context, var materialId: Int) : View(context) {
    var texture: Bitmap?=null
        set(value){field=value;invalidate()}
    private val paint=Paint(Paint.ANTI_ALIAS_FLAG or Paint.FILTER_BITMAP_FLAG)
    override fun onDraw(c: Canvas){
        c.save();val size=minOf(width,height).toFloat();c.translate((width-size)/2,(height-size)/2);c.scale(size/100,size/100)
        fun face(points: FloatArray,shade: Int){
            val path=Path();path.moveTo(points[0],points[1]);for(i in 2..6 step 2)path.lineTo(points[i],points[i+1]);path.close()
            paint.color=when(materialId){5->0xff9b6d3b.toInt();2->0xffaa5948.toInt();3->0xff677983.toInt();else->0xffa7aaac.toInt()};c.drawPath(path,paint)
            texture?.let { image ->
                val src=floatArrayOf(0f,0f,image.width.toFloat(),0f,image.width.toFloat(),image.height.toFloat(),0f,image.height.toFloat())
                val matrix=Matrix();matrix.setPolyToPoly(src,0,points,0,4);c.save();c.clipPath(path);c.drawBitmap(image,matrix,paint);c.restore()
            }
            paint.color=Color.argb(shade,0,0,0);c.drawPath(path,paint)
            paint.style=Paint.Style.STROKE;paint.strokeWidth=1f;paint.color=0x88ffffff.toInt();c.drawPath(path,paint);paint.style=Paint.Style.FILL
        }
        face(floatArrayOf(50f,6f,90f,27f,50f,49f,10f,27f),0)
        face(floatArrayOf(10f,27f,50f,49f,50f,94f,10f,72f),25)
        face(floatArrayOf(50f,49f,90f,27f,90f,72f,50f,94f),65)
        c.restore()
    }
}
