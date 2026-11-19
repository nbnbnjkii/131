package com.example.realms
import org.junit.Assert.*
import org.junit.Test
class BuildCatalogTest {
    @Test fun picturesMatchNativeTypesWithoutDuplicateAssets() {
        val items=BuildCatalog.entries
        assertEquals(listOf(0,1,2,3,4,5,6,7,9,10,11,8,12),items.map { it.nativeType })
        assertEquals(13,items.map { it.asset }.toSet().size)
        assertEquals(listOf("Бетонный блок","Плита (пол)","Окно","Дверь","Стол","Стул","Шкаф","Диван","Деревянный блок","Кирпичный блок","Металлический блок","УАЗ-469","Куб 1×1×1"),items.map { it.label })
        assertEquals(listOf(1,1,1,5,5,5,5,6,5,2,3,-1,1),items.map { it.defaultSurface })
        assertTrue(items.all { it.asset.startsWith("catalog/") || it.asset.startsWith("textures/") })
    }
}
