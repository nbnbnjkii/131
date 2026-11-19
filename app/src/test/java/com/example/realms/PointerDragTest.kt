package com.example.realms
import org.junit.Assert.*
import org.junit.Test
class PointerDragTest {
    @Test fun simultaneousWalkingAndLookingHaveIndependentOwners() {
        val stick=PointerDrag();val camera=PointerDrag()
        stick.start(4,30f,50f);camera.start(9,300f,80f)
        assertEquals(Pair(0f,-20f),stick.move(4,30f,30f))
        assertEquals(Pair(40f,5f),camera.move(9,340f,85f))
        camera.end(4);assertEquals(9,camera.pointer)
        assertNull(stick.move(9,100f,100f));assertEquals(4,stick.pointer)
        camera.end(9);assertEquals(4,stick.pointer)
        assertEquals(Pair(10f,0f),stick.move(4,40f,30f))
        camera.start(11,350f,80f);stick.end(4)
        assertEquals(Pair(-10f,0f),camera.move(11,340f,80f))
        camera.reset();assertNull(camera.move(11,600f,80f))
    }
}
