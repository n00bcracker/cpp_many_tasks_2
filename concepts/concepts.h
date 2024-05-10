#pragma once

template <class P, class T>
concept Predicate = true;

template <class T>
concept Indexable = true;

template <class T>
concept SerializableToJson = true;
